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



namespace ofxMarkSynth {



void Gui::setup(std::shared_ptr<Synth> synthPtr_, std::shared_ptr<ofAppBaseWindow> windowPtr) {
  synthPtr = synthPtr_;
  
  imgui.setup(windowPtr);
  ImNodes::CreateContext();

  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  // Keep Viewports disabled so everything stays inside this window:
  // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
//  ImFontConfig config;
//  config.MergeMode = false;
//  io.Fonts->AddFontFromFileTTF("/System/Library/Fonts/Menlo.ttc", 16.0f, &config);
  
  ImGuiStyle& style = ImGui::GetStyle();
  style.WindowRounding = 4.f;
  
//  gui.add(activeIntentInfoLabel1.setup("I1", ""));
//  gui.add(activeIntentInfoLabel2.setup("I2", ""));
}

void Gui::exit() {
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

// TODO: Replace with ofxSurfingImGui log window module
void Gui::drawLog() {
  ImGui::Begin("Log");
  ImGui::BeginChild("LogScrollingRegion", ImVec2(0,0), false, ImGuiWindowFlags_HorizontalScrollbar);
  ImGui::TextUnformatted(ofToString(ofGetFrameRate()).c_str());
  ImGui::EndChild();
  ImGui::End();
}

const char* PAUSE_ICON = "||";
const char* PLAY_ICON = "> ";
auto RED_COLOR = ImVec4(0.9,0.2,0.2,1);
auto GREEN_COLOR = ImVec4(0.2,0.6,0.3,1);
auto YELLOW_COLOR = ImVec4(0.9,0.9,0.2,1);
auto GREY_COLOR = ImVec4(0.5,0.5,0.5,1);

void Gui::drawSynthControls() {
  ImGui::Begin("Synth");
  
  addParameterGroup(synthPtr->getParameterGroup());
  
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

void Gui::drawVerticalSliders(ofParameterGroup& paramGroup) {
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 8)); // tighter spacing
  const ImVec2 sliderSize(24, 140);
  const float colW = sliderSize.x + 0.0f;   // column width (slider + padding)
  const float colH = sliderSize.y + 28.0f;   // add room for label below

  if (ImGui::BeginTable(paramGroup.getName().c_str(), paramGroup.size(),
                        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX)) {
    
    for (int i = 0; i < paramGroup.size(); ++i) {
      ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, colW);
    }
    ImGui::TableNextRow();
    
    for (int i = 0; i < paramGroup.size(); ++i) {
      const auto& name = paramGroup[i].getName();
      
      ImGui::TableSetColumnIndex(i);
      ImGui::PushID(i);
      
      ImGui::BeginGroup();
      // center slider within the fixed column
      float xPad = (colW - sliderSize.x) * 0.5f;
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + xPad);
      
      // copy current value to a local so VSliderFloat can edit it by pointer
      float v = paramGroup[i].cast<float>().get();
      if (ImGui::VSliderFloat("##v", sliderSize, &v, 0.0, 1.0, "%.1f")) {
        paramGroup[i].cast<float>().set(v);
      }
      ImGui::SetItemTooltip("%s", name.c_str());
      
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() - xPad);
      ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + colW);
      ImGui::TextWrapped("%s", name.substr(0, 3).c_str());
      ImGui::PopTextWrapPos();
      ImGui::EndGroup();
      
      ImGui::PopID();
    }
    
    ImGui::EndTable();
  }
  ImGui::PopStyleVar();
}

constexpr float sliderWidth = 200.0f;

void Gui::addParameter(ofParameter<int>& parameter) {
  const auto& name = parameter.getName();
  int value = parameter.get();
  
  ImGui::PushItemWidth(sliderWidth);
  if (ImGui::SliderInt(("##" + name).c_str(), &value, parameter.getMin(), parameter.getMax())) {
    parameter.set(value);
  }
  ImGui::SetItemTooltip("%s", name.c_str());
  ImGui::PopItemWidth();
  ImGui::SameLine();
  ImGui::Text("%s", parameter.getName().c_str());
}

void Gui::addParameter(ofParameter<float>& parameter) {
  const auto& name = parameter.getName();
  float value = parameter.get();
  
  ImGui::PushItemWidth(sliderWidth);
  if (ImGui::SliderFloat(("##" + name).c_str(), &value, parameter.getMin(), parameter.getMax(), "%.2f")) {
    parameter.set(value);
  }
  ImGui::SetItemTooltip("%s", name.c_str());
  ImGui::PopItemWidth();
  ImGui::SameLine();
  ImGui::Text("%s", parameter.getName().c_str());
}

void Gui::addParameter(ofParameter<ofFloatColor>& parameter) {
  const auto& name = parameter.getName();
  ofFloatColor color = parameter.get();
  float colorArray[4] = { color.r, color.g, color.b, color.a };
  
  ImGui::PushItemWidth(sliderWidth);
  if (ImGui::ColorEdit4(("##" + name).c_str(), colorArray, ImGuiColorEditFlags_Float)) {
    parameter.set(ofFloatColor(colorArray[0], colorArray[1], colorArray[2], colorArray[3]));
  }
  ImGui::SetItemTooltip("%s", name.c_str());
  ImGui::PopItemWidth();
  ImGui::SameLine();
  ImGui::Text("%s", parameter.getName().c_str());
}

void Gui::addParameter(ofParameter<glm::vec2>& parameter) {
  const auto& name = parameter.getName();
  glm::vec2 value = parameter.get();
  float valueArray[2] = { value.x, value.y };

  ImGui::PushItemWidth(sliderWidth);
  if (ImGui::SliderFloat2(("##" + name).c_str(), valueArray, parameter.getMin().x, parameter.getMax().x, "%.2f")) {
    parameter.set(glm::vec2(valueArray[0], valueArray[1]));
  }
  ImGui::SetItemTooltip("%s", name.c_str());
  ImGui::PopItemWidth();
  ImGui::SameLine();
  ImGui::Text("%s", parameter.getName().c_str());
}

void Gui::addParameter(ofAbstractParameter& parameter) {
  if (parameter.type() == typeid(ofParameterGroup).name()) {
    if (ImGui::TreeNode(parameter.getName().c_str())) {
      addParameterGroup(parameter.castGroup());
      ImGui::TreePop();
    }
  } else if (parameter.type() == typeid(ofParameter<int>).name()) {
    auto& intParam = parameter.cast<int>();
    addParameter(intParam);
  } else if (parameter.type() == typeid(ofParameter<float>).name()) {
    auto& floatParam = parameter.cast<float>();
    addParameter(floatParam);
  } else if (parameter.type() == typeid(ofParameter<ofFloatColor>).name()) {
    auto& colorParam = parameter.cast<ofFloatColor>();
    addParameter(colorParam);
  } else if (parameter.type() == typeid(ofParameter<glm::vec2>).name()) {
    auto& colorParam = parameter.cast<glm::vec2>();
    addParameter(colorParam);
  } else {
    ImGui::Text("Unsupported parameter type: %s", parameter.type().c_str());
  }
}

void Gui::addParameterGroup(ofParameterGroup& paramGroup) {
  for (auto& parameterPtr : paramGroup) {
    addParameter(*parameterPtr);
  }
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

  addParameter(synthPtr->exposureParameter);
  addParameter(synthPtr->gammaParameter);
  addParameter(synthPtr->whitePointParameter);
  addParameter(synthPtr->contrastParameter);
  addParameter(synthPtr->saturationParameter);
  addParameter(synthPtr->brightnessParameter);
  addParameter(synthPtr->hueShiftParameter);
  addParameter(synthPtr->sideExposureParameter);
}

constexpr float thumbW = 128.0f;
constexpr ImVec2 thumbSize(thumbW, thumbW);

void Gui::drawInternalState() {
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
  ImGui::Text("%s FPS", ofToString(ofGetFrameRate(), 0).c_str());
  
  if (synthPtr->paused) {
    ImGui::TextColored(YELLOW_COLOR, "%s Paused", PAUSE_ICON);
  } else {
    ImGui::TextColored(GREY_COLOR, "%s Playing", PLAY_ICON);
  }
  
#ifdef TARGET_MAC
  if (synthPtr->recorder.isRecording()) {
    ImGui::TextColored(ImVec4(0.9,0.2,0.2,1), "<> Recording");
  } else {
    ImGui::TextColored(GREY_COLOR, "   Not Recording");
  }
#endif
  
  if (SaveToFileThread::activeThreadCount < 1) {
    ImGui::TextColored(GREY_COLOR, "   No Image Saves");
  } else {
    ImGui::TextColored(YELLOW_COLOR, ">> %d Image Saves", SaveToFileThread::activeThreadCount);
  }
}

void Gui::drawNodeEditor() {
  // Rebuild node model if dirty
  if (nodeEditorDirty) {
    nodeEditorModel.buildFromSynth(synthPtr);
    nodeEditorDirty = false;
    layoutComputed = false;
    layoutAutoLoadAttempted = false; // Reset auto-load on rebuild
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
  
  // Toolbar for layout controls
  if (ImGui::Button("Compute Layout")) {
    nodeEditorModel.computeLayout();
    layoutComputed = true;
    animateLayout = false;
  }
  ImGui::SameLine();
  if (ImGui::Button("Animate Layout")) {
    nodeEditorModel.resetLayout();
    layoutComputed = false;
    animateLayout = true;
  }
  ImGui::SameLine();
  ImGui::Checkbox("Auto-animate", &animateLayout);
  
  ImGui::SameLine();
  ImGui::Text("|");
  
  // Save/Load buttons
  ImGui::SameLine();
  if (ImGui::Button("Save Layout")) {
      // Sync current positions from imnodes before saving
      nodeEditorModel.syncPositionsFromImNodes();
      if (nodeEditorModel.saveLayout()) {
          ofLogNotice("Gui") << "Saved node layout for: " << synthPtr->name;
      } else {
          ofLogError("Gui") << "Failed to save node layout";
      }
  }
  
  ImGui::SameLine();
  if (ImGui::Button("Load Layout")) {
      if (nodeEditorModel.loadLayout()) {
          layoutComputed = true;
          animateLayout = false;
          ofLogNotice("Gui") << "Loaded node layout for: " << synthPtr->name;
      } else {
          ofLogError("Gui") << "Failed to load node layout";
      }
  }
  
  // Status indicators
  if (nodeEditorModel.hasStoredLayout()) {
      ImGui::SameLine();
      ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "[Saved]");
  }
  
  if (nodeEditorModel.isLayoutAnimating()) {
    ImGui::SameLine();
    ImGui::TextColored(GREEN_COLOR, "[Animating...]");
  }
  
  ImGui::Separator();
  
  // Run animated layout if enabled and not yet computed
  if (animateLayout && !layoutComputed) {
    nodeEditorModel.computeLayoutAnimated();
    if (!nodeEditorModel.isLayoutAnimating()) {
      layoutComputed = true; // Animation finished
    }
  }
  
  // Draw the node editor
  ImNodes::BeginNodeEditor();
  
  ImNodesIO& io = ImNodes::GetIO();
  io.EmulateThreeButtonMouse.Modifier = &ImGui::GetIO().KeyAlt; // Option-drag to pan

  // Draw nodes
  for (const auto& node : nodeEditorModel.nodes) {
    ModPtr modPtr = node.modPtr;
    int modId = modPtr->getId();
    
    ImNodes::BeginNode(node.modPtr->getId());
    
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted(modPtr->name.c_str());
    ImGui::ProgressBar(modPtr->getAgency(), ImVec2(64.0f, 4.0f), "");
    ImNodes::EndNodeTitleBar();
    
    // Input attributes (sinks)
    for (const auto& [name, id] : modPtr->sinkNameIdMap) {
      ImNodes::BeginInputAttribute(NodeEditorModel::sinkId(modId, id));
      if (!modPtr->parameters.contains(name)) {
        ImGui::TextUnformatted(name.c_str());
      } else {
        auto& p = modPtr->parameters.get(name);
        addParameter(p);
      }
      ImNodes::EndInputAttribute();
    }

    for (auto& parameter : modPtr->parameters) {
      if (!modPtr->sinkNameIdMap.contains(parameter->getName())) {
        addParameter(*parameter);
      }
    }

    // Output attributes (sources)
    for (const auto& [name, id] : modPtr->sourceNameIdMap) {
      ImNodes::BeginOutputAttribute(NodeEditorModel::sourceId(modId, id));
      ImGui::TextUnformatted(name.c_str());
      ImNodes::EndOutputAttribute();
    }
      
    ImGui::Dummy(ImVec2(0.0f, 0.0f));

    ImNodes::EndNode();
  }
  
  // Draw links (connections)
  int linkId = 0; // TODO: make this stable when we make the node editor editable
  for (const auto& node : nodeEditorModel.nodes) {
    ModPtr modPtr = node.modPtr;
    int sourceModId = modPtr->getId();

    for (const auto& [sourceId, sinksPtr] : modPtr->connections) {
      for (const auto& [sinkModPtr, sinkId] : *sinksPtr) {
        int sinkModId = sinkModPtr->getId();
        ImNodes::Link(linkId++,
                      NodeEditorModel::sourceId(sourceModId, sourceId),
                      NodeEditorModel::sinkId(sinkModId, sinkId));
      }
    }
  }
  
  ImNodes::MiniMap(0.2f, ImNodesMiniMapLocation_BottomRight);

  ImNodes::EndNodeEditor();
  
  // Sync positions from imnodes back to model after every frame
  // This ensures manual dragging is captured
  nodeEditorModel.syncPositionsFromImNodes();
  
  ImGui::End();
}




} // namespace ofxMarkSynth
