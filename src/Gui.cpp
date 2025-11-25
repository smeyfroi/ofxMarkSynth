//
//  Gui.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 05/11/2025.
//

#include "Gui.hpp"
#include "Synth.hpp"
#include "imgui_internal.h" // for DockBuilder
#include "ofxTimeMeasurements.h"
#include "imnodes.h"
#include "ImGuiUtil.hpp"
#include "NodeRenderUtil.hpp"



// TODO: FBO handling is more complicated: a Mod can have a set of layers that it can draw on



namespace ofxMarkSynth {
using namespace NodeRenderUtil;

// Unicode icons for GUI controls
const char* PLAY_ICON = "\xE2\x96\xB6";   // ▶ U+25B6 Black Right-Pointing Triangle
const char* PAUSE_ICON = "\xE2\x80\x96";  // ‖ U+2016 Double Vertical Line
const char* RESET_ICON = "\xE2\x86\xBB";  // ↻ U+21BB Clockwise Open Circle Arrow
const char* RECORD_ICON = "\xE2\x97\x8F"; // ● U+25CF Black Circle
const char* SAVE_ICON = "\xE2\x86\x93";   // ↓ U+2193 Downwards Arrow
const char* LOAD_ICON = "\xE2\x86\x91";   // ↑ U+2191 Upwards Arrow
const char* CLEAR_ICON = "\xE2\x9C\x96";  // ✖ U+2716 Heavy Multiplication X
const char* SHUFFLE_ICON = "\xE2\x86\xBB"; // ↻ U+21BB Clockwise Open Circle Arrow (reuse)

auto RED_COLOR = ImVec4(0.9,0.2,0.2,1);
auto GREEN_COLOR = ImVec4(0.2,0.6,0.3,1);
auto YELLOW_COLOR = ImVec4(0.9,0.9,0.2,1);
auto GREY_COLOR = ImVec4(0.5,0.5,0.5,1);

void Gui::setup(std::shared_ptr<Synth> synthPtr_, std::shared_ptr<ofAppBaseWindow> windowPtr) {
  synthPtr = synthPtr_;
  
  imgui.setup(windowPtr);
  ImNodes::CreateContext();

  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  // Keep Viewports disabled so everything stays inside this window:
  // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
  
  // Load font with Unicode icon support
  ImFontConfig fontConfig;
  fontConfig.OversampleH = 2;
  fontConfig.OversampleV = 2;
  
  // Define character ranges we need
  static const ImWchar ranges[] = {
    0x0020, 0x00FF, // Basic Latin + Latin Supplement
    0x2010, 0x2027, // General Punctuation (includes ‖ U+2016)
    0x2190, 0x21FF, // Arrows (includes ↻ U+21BB, ↑ U+2191, ↓ U+2193, ↶ U+21B6, ↷ U+21B7)
    0x2700, 0x27BF, // Dingbats (includes ✓ U+2713, ✖ U+2716, ✗ U+2717)
    0x2900, 0x297F, // Supplemental Arrows-B (includes ⤮ U+292E)
    0x25A0, 0x25FF, // Geometric Shapes (includes ▶ U+25B6, ● U+25CF)
    0x2B00, 0x2BFF, // Miscellaneous Symbols and Arrows (includes ⬆ U+2B06, ⬇ U+2B07)
    0,
  };
  
  ImFont* font = imgui.addFont("Arial Unicode.ttf", 18.0f, &fontConfig, ranges, true);
  if (!font) {
    ofLogWarning("Gui") << "Failed to load Arial Unicode.ttf, using default font";
  }
  
  ImGuiStyle& style = ImGui::GetStyle();
  style.WindowRounding = 4.f;
  
  // Initialize performance timer
  timerStartTime = ofGetElapsedTimef();
  timerPausedTime = 0.0f;
  timerTotalPausedDuration = 0.0f;
  timerPaused = false;
  
//  gui.add(activeIntentInfoLabel1.setup("I1", ""));
//  gui.add(activeIntentInfoLabel2.setup("I2", ""));
}

void Gui::exit() {
  if (!synthPtr) return; // not used by a Synth
  
  ImNodes::DestroyContext();
  imgui.exit();
}

void Gui::draw() {
  TS_START("Gui::draw");
  TSGL_START("Gui::draw");
  
  imgui.begin();
  
  drawDockspace();
  drawLog();
  drawSynthControls();
  drawNodeEditor();
  
  imgui.end();
  imgui.draw();
  
  TSGL_STOP("Gui::draw");
  TS_STOP("Gui::draw");
}

void Gui::drawDockspace() {
  // Fullscreen invisible window to contain the DockSpace
  ImGuiWindowFlags hostFlags =
      ImGuiWindowFlags_NoDocking |
      ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoBringToFrontOnFocus |
      ImGuiWindowFlags_NoNavFocus |
      ImGuiWindowFlags_MenuBar;

  ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->Pos);
  ImGui::SetNextWindowSize(viewport->Size);
  ImGui::SetNextWindowViewport(viewport->ID);

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::Begin("DockHost", nullptr, hostFlags);
  ImGui::PopStyleVar(2);

  ImGuiID dockspaceId = ImGui::GetID("DockSpace");
  ImGuiDockNodeFlags dockFlags = ImGuiDockNodeFlags_PassthruCentralNode;
  ImGui::DockSpace(dockspaceId, ImVec2(0,0), dockFlags);
  if (!dockBuilt) {
    buildInitialDockLayout(dockspaceId);
    dockBuilt = true;
  }
  ImGui::End();
}

void Gui::buildInitialDockLayout(ImGuiID dockspaceId) {
  ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
  ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->Size);
  
  ImGuiID dockMain = dockspaceId;
  ImGuiID dockLeft, dockRight, dockBottom, dockCenter;
  
  ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down,  0.15f, &dockBottom, &dockMain);
  ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.30f, &dockRight, &dockCenter);
  
  ImGui::DockBuilderDockWindow("Synth", dockRight);
  ImGui::DockBuilderDockWindow("Log", dockBottom);
  ImGui::DockBuilderDockWindow("NodeEditor", dockCenter);
  
  ImGui::DockBuilderFinish(dockspaceId);
}

void Gui::drawLog() {
  static ImGuiTextFilter filter;
  static bool autoScroll = true;
  static bool scrollToBottom = false;
  
  if (ImGui::Begin("Log")) {
    auto& logger = synthPtr->loggerChannelPtr;
    
    if (ImGui::Button((std::string(CLEAR_ICON) + " Clear").c_str()) && logger) logger->clear();
    ImGui::SameLine();
    if (ImGui::Button("Copy")) ImGui::LogToClipboard();
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &autoScroll);
    filter.Draw("Filter", 180);
    ImGui::Separator();
    
    ImGui::BeginChild(
                      "LogScrollRegion",
                      ImVec2(0, 0),
                      true,
                      ImGuiWindowFlags_HorizontalScrollbar
                      );
    
    for (auto &l : logger->getLogs()) {
      if (!filter.PassFilter(l.message.c_str())) continue;
      
      ImVec4 color;
      switch (l.level) {
        case OF_LOG_VERBOSE: color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f); break;
        case OF_LOG_NOTICE:  color = ImVec4(0.8f, 0.9f, 1.0f, 1.0f); break;
        case OF_LOG_WARNING: color = ImVec4(1.0f, 0.8f, 0.3f, 1.0f); break;
        case OF_LOG_ERROR:   color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); break;
        case OF_LOG_FATAL_ERROR:
          color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); break;
        default:             color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break;
      }
      
      ImGui::PushStyleColor(ImGuiCol_Text, color);
      ImGui::TextUnformatted(l.message.c_str());
      ImGui::PopStyleColor();
    }
    
    if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) scrollToBottom = true;
    
    if (scrollToBottom) {
      ImGui::SetScrollHereY(1.0f);  // 1.0 = bottom
      scrollToBottom = false;
    }
    
    ImGui::EndChild();
  }
  ImGui::End();
}

void Gui::drawSynthControls() {
  ImGui::Begin("Synth");
  
  addParameterGroup(synthPtr, synthPtr->getParameterGroup());
  
  ImGui::SeparatorText("Intents");
  drawIntentControls();
  
  ImGui::SeparatorText("Layers");
  drawLayerControls();
  
  ImGui::SeparatorText("Display");
  drawDisplayControls();
  
  ImGui::SeparatorText("State");
  drawInternalState();
  
  ImGui::SeparatorText("Status");
  drawStatus();
  
  ImGui::End();
}

void Gui::drawIntentControls() {
  drawVerticalSliders(synthPtr->intentParameters);
}

void Gui::drawLayerControls() {
  drawVerticalSliders(synthPtr->fboParameters);
}

void Gui::drawDisplayControls() {
  ofxImGui::Settings settings = ofxImGui::Settings();
  
  const char* tonemapOptions[] = {
    "Linear (clamp)",
    "Reinhard",
    "Reinhard Extended",
    "ACES",
    "Filmic",
    "Exposure"
  };
  int currentTonemap = synthPtr->toneMapTypeParameter.get();
  ImGui::PushItemWidth(150.0f);
  if (ImGui::Combo("##tonemap", &currentTonemap, tonemapOptions, IM_ARRAYSIZE(tonemapOptions))) {
    synthPtr->displayParameters[2].cast<int>().set(currentTonemap);
  }
  ImGui::PopItemWidth();
  ImGui::SameLine();
  ImGui::Text("%s", synthPtr->toneMapTypeParameter.getName().c_str());

  addParameter(synthPtr, synthPtr->exposureParameter);
  addParameter(synthPtr, synthPtr->gammaParameter);
  addParameter(synthPtr, synthPtr->whitePointParameter);
  addParameter(synthPtr, synthPtr->contrastParameter);
  addParameter(synthPtr, synthPtr->saturationParameter);
  addParameter(synthPtr, synthPtr->brightnessParameter);
  addParameter(synthPtr, synthPtr->hueShiftParameter);
  addParameter(synthPtr, synthPtr->sideExposureParameter);
}

constexpr float thumbW = 128.0f;
constexpr ImVec2 thumbSize(thumbW, thumbW);

void Gui::drawInternalState() {
  if (synthPtr->liveTexturePtrFns.size() == 0) return;
  
  // Calculate required height: text + image + padding
  const float contentHeight = ImGui::GetTextLineHeightWithSpacing() + thumbW + ImGui::GetStyle().ItemSpacing.y + 20.0f;
  
  ImGui::BeginChild("tex_scroll", ImVec2(0, contentHeight), true, ImGuiWindowFlags_HorizontalScrollbar);

  if (ImGui::BeginTable("##textures", synthPtr->liveTexturePtrFns.size(),
                        ImGuiTableFlags_SizingFixedFit |
                        ImGuiTableFlags_ScrollX |
                        ImGuiTableFlags_NoHostExtendX)) {
    
    // Set up columns with fixed width
    for (size_t i = 0; i < synthPtr->liveTexturePtrFns.size(); ++i) {
      ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, thumbW + 8.0f);
    }
    
    ImGui::TableNextRow();

    int colIndex = 0;
    for (const auto& tex : synthPtr->liveTexturePtrFns) {
      ImGui::TableSetColumnIndex(colIndex++);
      
      ImGui::Text("%s", tex.first.c_str());
      const ofTexture* texturePtr = tex.second();
      if (!texturePtr || !texturePtr->isAllocated()) {
        ImGui::Dummy(ImVec2(thumbW, thumbW));
        continue;
      }

      const auto& textureData = texturePtr->getTextureData();
      assert(textureData.textureTarget == GL_TEXTURE_2D);
      ImTextureID imguiTexId = (ImTextureID)(uintptr_t)textureData.textureID;

      ImGui::PushID(tex.first.c_str());
      ImGui::Image(imguiTexId, thumbSize);
      ImGui::PopID();
    }

    ImGui::EndTable();
  }

  ImGui::EndChild();
}

void Gui::drawStatus() {  
  if (!synthPtr->currentConfigPath.empty()) {
    std::filesystem::path p(synthPtr->currentConfigPath);
    std::string filename = p.filename().string();
    ImGui::TextColored(GREY_COLOR, "Config: %s", filename.c_str());
  } else {
    ImGui::TextColored(GREY_COLOR, "Config: None");
  }
  
  // Performance timer display
  float currentTime = ofGetElapsedTimef();
  float elapsedTime;
  
  if (timerPaused) {
    elapsedTime = timerPausedTime - timerStartTime - timerTotalPausedDuration;
  } else {
    elapsedTime = currentTime - timerStartTime - timerTotalPausedDuration;
  }
  
  int minutes = static_cast<int>(elapsedTime) / 60;
  int seconds = static_cast<int>(elapsedTime) % 60;
  
  ImGui::Text("Timer: %02d:%02d", minutes, seconds);
  ImGui::SameLine();
  
  // Pause/Resume button
  if (timerPaused) {
    if (ImGui::SmallButton(PLAY_ICON)) {
      timerTotalPausedDuration += (currentTime - timerPausedTime);
      timerPaused = false;
    }
  } else {
    if (ImGui::SmallButton(PAUSE_ICON)) {
      timerPausedTime = currentTime;
      timerPaused = true;
    }
  }
  
  ImGui::SameLine();
  
  // Reset button - sets timer back to 00:00
  if (ImGui::SmallButton(RESET_ICON)) {
    timerStartTime = currentTime;
    timerTotalPausedDuration = 0.0f;
    if (timerPaused) {
      timerPausedTime = currentTime;  // Keep paused at 00:00
    }
  }
  
  // FPS counter on same line with spacing
  ImGui::SameLine(0.0f, 20.0f);  // 20 pixels spacing
  ImGui::Text("%s FPS", ofToString(ofGetFrameRate(), 0).c_str());
  
  if (synthPtr->paused) {
    ImGui::TextColored(YELLOW_COLOR, "%s Paused", PAUSE_ICON);
  } else {
    ImGui::TextColored(GREY_COLOR, "%s Playing", PLAY_ICON);
  }
  
#ifdef TARGET_MAC
  if (synthPtr->recorder.isRecording()) {
    ImGui::TextColored(RED_COLOR, "%s Recording", RECORD_ICON);
  } else {
    ImGui::TextColored(GREY_COLOR, "   Not Recording");
  }
#endif
  
  if (SaveToFileThread::activeThreadCount < 1) {
    ImGui::TextColored(GREY_COLOR, "   No Image Saves");
  } else {
    ImGui::TextColored(YELLOW_COLOR, "%s %d Image Saves", SAVE_ICON, SaveToFileThread::activeThreadCount);
  }
}

constexpr int FBO_PARAMETER_ID = 0;
void Gui::drawNode(const ModPtr& modPtr, bool highlight) {
  int modId = modPtr->getId();
  
  if (highlight) {
    ImNodes::PushColorStyle(ImNodesCol_TitleBar, IM_COL32(50, 200, 100, 255));
    ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, IM_COL32(70, 220, 120, 255));
    ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, IM_COL32(90, 240, 140, 255));
  }
  
  ImNodes::BeginNode(modId);

  ImNodes::BeginNodeTitleBar();
  ImGui::TextUnformatted(modPtr->getName().c_str());
  ImGui::ProgressBar(modPtr->getAgency(), ImVec2(64.0f, 4.0f), "");
  ImNodes::EndNodeTitleBar();
  
  // Input attributes (sinks)
  for (const auto& [name, id] : modPtr->sinkNameIdMap) {
    ImNodes::BeginInputAttribute(NodeEditorModel::sinkId(modId, id));
    if (!modPtr->parameters.contains(name)) {
      ImGui::TextUnformatted(name.c_str());
    } else {
      auto& p = modPtr->parameters.get(name);
      addParameter(modPtr, p);
    }
    ImNodes::EndInputAttribute();
  }
  ImNodes::BeginInputAttribute(NodeEditorModel::sinkId(modId, FBO_PARAMETER_ID));
  ImGui::TextUnformatted("FBO");
  ImNodes::EndInputAttribute();

  // Parameters without sinks
  for (auto& parameter : modPtr->parameters) {
    if (!modPtr->sinkNameIdMap.contains(parameter->getName())) {
      addParameter(modPtr, *parameter);
    }
  }

  // Output attributes (sources)
  for (const auto& [name, id] : modPtr->sourceNameIdMap) {
    ImNodes::BeginOutputAttribute(NodeEditorModel::sourceId(modId, id));
    ImGui::TextUnformatted(name.c_str());
    ImNodes::EndOutputAttribute();
  }
  
  ImNodes::EndNode();
  
  // Pop highlight colors
  if (highlight) {
    ImNodes::PopColorStyle();
    ImNodes::PopColorStyle();
    ImNodes::PopColorStyle();
  }
}

void Gui::drawNode(const DrawingLayerPtr& layerPtr) {
  int layerId = layerPtr->id;
  
  ImNodes::PushColorStyle(ImNodesCol_TitleBar, IM_COL32(128, 128, 50, 255));
  ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, IM_COL32(128, 128, 75, 255));
  ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, IM_COL32(128, 128, 100, 255));

  ImNodes::BeginNode(layerId);

  ImNodes::BeginNodeTitleBar();
  ImGui::TextUnformatted(layerPtr->name.c_str());
  ImNodes::EndNodeTitleBar();
  
  ImNodes::BeginOutputAttribute(NodeEditorModel::sourceId(layerId, FBO_PARAMETER_ID));
  ImGui::TextUnformatted("FBO");
  ImNodes::EndInputAttribute();
  
  ImNodes::EndNode();
  
  ImNodes::PopColorStyle();
  ImNodes::PopColorStyle();
  ImNodes::PopColorStyle();
}

void Gui::drawNodeEditor() {
  // Rebuild node model if dirty
  if (nodeEditorDirty) {
    nodeEditorModel.buildFromSynth(synthPtr);
    nodeEditorDirty = false;
    layoutComputed = false;
    layoutAutoLoadAttempted = false; // Reset auto-load on rebuild
    snapshotsLoaded = false; // Reload snapshots on rebuild
  }

  ImGui::Begin("NodeEditor");
  
  // Auto-load saved layout on first draw (if it exists)
  if (!layoutAutoLoadAttempted) {
    layoutAutoLoadAttempted = true;
    if (nodeEditorModel.hasStoredLayout()) {
      if (nodeEditorModel.loadLayout()) {
        layoutComputed = true;
        animateLayout = false; // Don't animate if we loaded positions
        ofLogNotice("Gui") << "Auto-loaded node layout for: " << synthPtr->name;
      }
    }
  }
  
  // Auto-load snapshots on first draw
  if (!snapshotsLoaded) {
    snapshotManager.loadFromFile(synthPtr->getName());
    snapshotsLoaded = true;
  }
  
  // Clear highlights after timeout
  if (!highlightedMods.empty()) {
    float elapsed = ofGetElapsedTimef() - highlightStartTime;
    if (elapsed > HIGHLIGHT_DURATION) {
      highlightedMods.clear();
    }
  }
  
  if (ImGui::Button((std::string(SHUFFLE_ICON) + " Random Layout").c_str())) {
    nodeEditorModel.resetLayout();
    layoutComputed = false;
    animateLayout = true;
  }
  ImGui::SameLine();
  ImGui::Text("|");
  
  // Save/Load buttons
  ImGui::SameLine();
  if (ImGui::Button((std::string(SAVE_ICON) + " Save Layout").c_str())) {
    // Sync current positions from imnodes before saving
    nodeEditorModel.syncPositionsFromImNodes();
    if (nodeEditorModel.saveLayout()) {
      ofLogNotice("Gui") << "Saved node layout for: " << synthPtr->name;
    } else {
      ofLogError("Gui") << "Failed to save node layout";
    }
  }
  ImGui::SameLine();
  if (ImGui::Button((std::string(LOAD_ICON) + " Load Layout").c_str())) {
    if (nodeEditorModel.loadLayout()) {
      layoutComputed = true;
      animateLayout = false;
      ofLogNotice("Gui") << "Loaded node layout for: " << synthPtr->name;
    } else {
      ofLogError("Gui") << "Failed to load node layout";
    }
  }
  
  // Run animated layout if enabled and not yet computed
  if (animateLayout && !layoutComputed) {
    nodeEditorModel.computeLayoutAnimated();
    if (!nodeEditorModel.isLayoutAnimating()) {
      layoutComputed = true; // Animation finished
    }
  }
  
  // Draw snapshot controls
  drawSnapshotControls();
  
  // Draw the node editor
  ImNodes::BeginNodeEditor();
  
  ImNodesIO& io = ImNodes::GetIO();
  io.EmulateThreeButtonMouse.Modifier = &ImGui::GetIO().KeyAlt; // Option-drag to pan

  // Draw nodes
  for (const auto& node : nodeEditorModel.nodes) {
    const auto& objectPtr = node.objectPtr;
    if (const auto modPtrPtr = std::get_if<ModPtr>(&objectPtr)) {
      bool highlight = highlightedMods.contains((*modPtrPtr)->getName());
      drawNode(*modPtrPtr, highlight);
    } else if (const auto drawingLayerPtrPtr = std::get_if<DrawingLayerPtr>(&objectPtr)) {
      drawNode(*drawingLayerPtrPtr);
    } else {
      ofLogError("Gui") << "Gui draw node with unknown objectPtr type";
    }
  }
  
  // Draw links
  int linkId = 0; // TODO: make this stable when we make the node editor editable
  
  for (const auto& node : nodeEditorModel.nodes) {
    const auto& objectPtr = node.objectPtr;
    if (const auto modPtrPtr = std::get_if<ModPtr>(&objectPtr)) {
      ModPtr modPtr = *modPtrPtr;
      int sourceModId = modPtr->getId();

      for (const auto& [sourceId, sinksPtr] : modPtr->connections) {
        for (const auto& [sinkModPtr, sinkId] : *sinksPtr) {
          int sinkModId = sinkModPtr->getId();
          bool isConnectedToSelection = ImNodes::IsNodeSelected(sourceModId) || ImNodes::IsNodeSelected(sinkModId);

          if (isConnectedToSelection) {
            ImNodes::PushColorStyle(ImNodesCol_Link, IM_COL32(100, 255, 100, 255));
          }
          
          ImNodes::Link(linkId++,
                        NodeEditorModel::sourceId(sourceModId, sourceId),
                        NodeEditorModel::sinkId(sinkModId, sinkId));
          
          if (isConnectedToSelection) {
            ImNodes::PopColorStyle();
          }
        }
      }
      
      // Connect FROM the Layer to the Mod
      for (const auto& [layerName, layerPtrs] : modPtr->namedDrawingLayerPtrs) {
        for (const auto& layerPtr : layerPtrs) {
          int layerNodeId = layerPtr->id;
          int alpha = 64;
          auto currentLayerPtr = modPtr->getCurrentNamedDrawingLayerPtr(layerName);
          if (currentLayerPtr && currentLayerPtr->get()->id == layerPtr->id) alpha = 255;
          bool isConnectedToSelection = ImNodes::IsNodeSelected(sourceModId) || ImNodes::IsNodeSelected(layerNodeId);
          if (isConnectedToSelection) {
            ImNodes::PushColorStyle(ImNodesCol_Link, IM_COL32(255, 255, 100, alpha));
          } else {
            ImNodes::PushColorStyle(ImNodesCol_Link, IM_COL32(200, 255, 50, alpha));
          }
          
          ImNodes::Link(linkId++,
                        NodeEditorModel::sourceId(layerNodeId, FBO_PARAMETER_ID),
                        NodeEditorModel::sinkId(sourceModId, FBO_PARAMETER_ID));
          
          ImNodes::PopColorStyle();
        }
      }
    }
  }
  
  ImNodes::MiniMap(0.2f, ImNodesMiniMapLocation_BottomRight);

  ImNodes::EndNodeEditor();
  
  // Sync positions from imnodes back to model after every frame to capture manual dragging
  nodeEditorModel.syncPositionsFromImNodes();
  
  ImGui::End();
}

std::vector<ModPtr> Gui::getSelectedMods() {
  std::vector<ModPtr> selectedMods;
  
  int numSelected = ImNodes::NumSelectedNodes();
  if (numSelected == 0) return selectedMods;
  
  std::vector<int> selectedIds(numSelected);
  ImNodes::GetSelectedNodes(selectedIds.data());
  
  // Convert node IDs to ModPtrs
  for (int nodeId : selectedIds) {
    for (const auto& node : nodeEditorModel.nodes) {
      if (const auto modPtrPtr = std::get_if<ModPtr>(&node.objectPtr)) {
        if ((*modPtrPtr)->getId() == nodeId) {
          selectedMods.push_back(*modPtrPtr);
          break;
        }
      }
    }
  }
  
  return selectedMods;
}

void Gui::drawSnapshotControls() {
  ImGui::Separator();
  
  // Get selected Mods
  auto selectedMods = getSelectedMods();
  bool hasSelection = !selectedMods.empty();
  bool hasName = strlen(snapshotNameBuffer) > 0;
  
  // Text input for snapshot name
  ImGui::SetNextItemWidth(120.0f);
  ImGui::InputTextWithHint("##SnapshotName", "snapshot name", snapshotNameBuffer, sizeof(snapshotNameBuffer));
  
  ImGui::SameLine();
  ImGui::Text("(%zu sel)", selectedMods.size());
  
  ImGui::SameLine();
  
  // Undo button
  if (snapshotManager.canUndo()) {
    if (ImGui::Button("Undo") || (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z))) {
      auto affected = snapshotManager.undo(synthPtr);
      highlightedMods = affected;
      highlightStartTime = ofGetElapsedTimef();
    }
  } else {
    ImGui::BeginDisabled();
    ImGui::Button("Undo");
    ImGui::EndDisabled();
  }
  
  // Snapshot slot buttons
  ImGui::Text("Snapshots:");
  ImGui::SameLine();
  
  for (int i = 0; i < ModSnapshotManager::NUM_SLOTS; ++i) {
    if (i > 0) ImGui::SameLine();
    
    bool occupied = snapshotManager.isSlotOccupied(i);
    std::string label = std::to_string(i + 1);
    
    // Determine button action
    bool canSave = hasName && hasSelection;
    bool canLoad = occupied && !hasName;
    bool canClear = occupied && hasName && !hasSelection; // Shift+click or type name without selection to clear
    
    // Color based on state
    if (occupied) {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.3f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 0.4f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.7f, 0.5f, 1.0f));
    }
    
    // Disable button if no valid action
    bool disabled = !canSave && !canLoad && !canClear;
    if (disabled) ImGui::BeginDisabled();
    
    ImGui::PushID(i);
    if (ImGui::Button(label.c_str(), ImVec2(28, 0))) {
      if (canSave) {
        // Save snapshot
        auto snapshot = snapshotManager.capture(snapshotNameBuffer, selectedMods);
        snapshotManager.saveToSlot(i, snapshot);
        snapshotManager.saveToFile(synthPtr->getName());
        snapshotNameBuffer[0] = '\0';  // Clear name
        ofLogNotice("Gui") << "Saved snapshot to slot " << (i + 1);
      } else if (canLoad) {
        // Load snapshot
        auto snapshot = snapshotManager.getSlot(i);
        if (snapshot) {
          auto affected = snapshotManager.apply(synthPtr, *snapshot);
          highlightedMods = affected;
          highlightStartTime = ofGetElapsedTimef();
          ofLogNotice("Gui") << "Loaded snapshot from slot " << (i + 1);
        }
      } else if (canClear) {
        // Clear slot (type a name without selection to enable clear)
        snapshotManager.clearSlot(i);
        snapshotManager.saveToFile(synthPtr->getName());
        snapshotNameBuffer[0] = '\0';  // Clear name
        ofLogNotice("Gui") << "Cleared snapshot slot " << (i + 1);
      }
    }
    ImGui::PopID();
    
    if (disabled) ImGui::EndDisabled();
    if (occupied) ImGui::PopStyleColor(3);
    
    // Tooltip for occupied slots
    if (occupied && ImGui::IsItemHovered()) {
      auto snapshot = snapshotManager.getSlot(i);
      if (snapshot) {
        ImGui::SetTooltip("%s\n%zu mods\n\n[Click to load]\n[Type name + click to clear]", 
                          snapshot->name.c_str(), 
                          snapshot->modParams.size());
      }
    } else if (!occupied && ImGui::IsItemHovered()) {
      ImGui::SetTooltip("[Select mods + type name + click to save]");
    }
  }
  
  // Keyboard shortcuts for loading slots (F1-F8 keys)
  for (int i = 0; i < ModSnapshotManager::NUM_SLOTS; ++i) {
    ImGuiKey key = static_cast<ImGuiKey>(ImGuiKey_F1 + i);
    if (ImGui::IsKeyPressed(key) && !ImGui::GetIO().WantTextInput) {
      if (snapshotManager.isSlotOccupied(i)) {
        auto snapshot = snapshotManager.getSlot(i);
        if (snapshot) {
          auto affected = snapshotManager.apply(synthPtr, *snapshot);
          highlightedMods = affected;
          highlightStartTime = ofGetElapsedTimef();
          ofLogNotice("Gui") << "Loaded snapshot from slot " << (i + 1) << " via F" << (i + 1);
        }
      }
    }
  }
}




} // namespace ofxMarkSynth
