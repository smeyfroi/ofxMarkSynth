//
//  Gui.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 05/11/2025.
//

#include "core/Gui.hpp"
#include "core/Synth.hpp"
#include "imgui_internal.h" // for DockBuilder
#include "ofxTimeMeasurements.h"
#include "imnodes.h"
#include "gui/ImGuiUtil.hpp"
#include "nodeEditor/NodeRenderUtil.hpp"
#include "gui/HelpContent.hpp"



// TODO: DrawingLayer handling is more complicated: a Mod can have a set of layers that it can draw on. We have the links so far, but no indication of which is active. A DrawingLayer can also not drawn (e.g. fluid velocities)



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
  // Disable ImGui keyboard navigation so arrow keys reach the Synth
  io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
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
  
  std::string fontPath = ofToDataPath("Arial Unicode.ttf", true);
  ImFont* font = nullptr;
  if (ofFile::doesFileExist(fontPath)) {
    font = imgui.addFont("Arial Unicode.ttf", 18.0f, &fontConfig, ranges, true);
    if (!font) {
      ofLogWarning("Gui") << "Failed to load Arial Unicode.ttf despite file existing, using default font";
    }
  } else {
    ofLogWarning("Gui") << "Font file Arial Unicode.ttf not found in data path, using default font";
  }
  
  // Use ImGui's default monospace font for help window and tooltips
  ImFontConfig monoConfig;
  monoConfig.SizePixels = 13.0f;
  monoFont = io.Fonts->AddFontDefault(&monoConfig);
  NodeRenderUtil::setMonoFont(monoFont);
  
  ImGuiStyle& style = ImGui::GetStyle();
  style.WindowRounding = 4.f;
}

void Gui::exit() {
  if (!synthPtr) return; // not used by a Synth
  
  ImNodes::DestroyContext();
  imgui.exit();
}

void Gui::onConfigLoaded() {
  // Reset node editor model and related GUI state for the new config
  nodeEditorModel = NodeEditorModel{};
  nodeEditorDirty = true;
  layoutComputed = false;
  layoutAutoLoadAttempted = false;
  layoutNeedsSave = false;
  modsConfigNeedsSave = false;
  snapshotsLoaded = false;
  highlightedMods.clear();
}


void Gui::draw() {
  TS_START("Gui::draw");
  TSGL_START("Gui::draw");
  
  imgui.begin();
  
  drawDockspace();
  drawLog();
  drawSynthControls();
  drawNodeEditor();
  drawHelpWindow();
  drawDebugView();
  
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
  ImGuiID dockLeft, dockRight, dockCenter;

  // First split right so the Synth pane keeps full height.
  ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.30f, &dockRight, &dockCenter);

  // Then split the center area down so Log is only as wide as NodeEditor.
  ImGuiID dockBottomCenter, dockCenterTop;
  ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Down, 0.15f, &dockBottomCenter, &dockCenterTop);

  ImGui::DockBuilderDockWindow("Synth", dockRight);
  ImGui::DockBuilderDockWindow("Log", dockBottomCenter);
  ImGui::DockBuilderDockWindow("NodeEditor", dockCenterTop);
  
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
    
    ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), true,
                      ImGuiWindowFlags_HorizontalScrollbar);
    
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
  
  drawStatus();
  
//  addParameterGroup(synthPtr, synthPtr->getParameterGroup());
  addParameter(synthPtr, synthPtr->agencyParameter);
  
  drawPerformanceNavigator();
  drawIntentControls();
  drawLayerControls();
  drawDisplayControls();
  drawInternalState();
  drawMemoryBank();
  
  ImGui::End();
}

void Gui::drawIntentSlotSliders() {
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 8)); // tighter spacing

  const ImVec2 sliderSize(24, 124);
  const float colW = sliderSize.x + 0.0f;

  constexpr int kActivationSlots = 7;
  constexpr int kTotalSlots = 8; // 7 intent slots + master

  if (ImGui::BeginTable("IntentSliders", kTotalSlots,
                        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX)) {
    for (int i = 0; i < kTotalSlots; ++i) {
      ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, colW);
    }
    ImGui::TableNextRow();

    // Intent activation slots (1..7) stay left-aligned.
    for (int i = 0; i < kActivationSlots; ++i) {
      ImGui::TableSetColumnIndex(i);
      ImGui::PushID(i);
      ImGui::BeginGroup();

      const auto& activationParams = synthPtr->intentController->getActivationParameters();
      const bool hasIntentParam =
        i < (int)activationParams.size() && activationParams[i];

      if (hasIntentParam) {
        auto& param = *activationParams[i];
        float v = param.get();
        if (ImGui::VSliderFloat("##v", sliderSize, &v, param.getMin(), param.getMax(), "%.1f", ImGuiSliderFlags_NoRoundToFormat)) {
          param.set(v);
        }
        ImGui::SetItemTooltip("%s", param.getName().c_str());
      } else {
        drawDisabledSlider(sliderSize, i);
      }

      ImGui::EndGroup();
      ImGui::PopID();
    }

    // Master intent strength is always the rightmost slot.
    ImGui::TableSetColumnIndex(kTotalSlots - 1);
    ImGui::PushID(kTotalSlots - 1);
    ImGui::BeginGroup();
    {
      // Access strength parameter through the intent controller's parameter group
      auto& intentParams = synthPtr->intentController->getParameterGroup();
      // The strength parameter is named "Intent Strength" and is always last in the group
      if (intentParams.contains("Intent Strength")) {
        auto& param = intentParams.getFloat("Intent Strength");
        float v = param.get();
        if (ImGui::VSliderFloat("##v", sliderSize, &v, param.getMin(), param.getMax(), "%.1f", ImGuiSliderFlags_NoRoundToFormat)) {
          param.set(v);
        }
        ImGui::SetItemTooltip("%s", param.getName().c_str());
      }

    }
    ImGui::EndGroup();
    ImGui::PopID();

    ImGui::EndTable();
  }

  ImGui::PopStyleVar();
}

void Gui::drawDisabledSlider(const ImVec2& size, int slotIndex) {
  ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f);
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.35f, 0.35f, 0.35f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.35f, 0.35f, 0.35f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.45f, 0.45f, 1.0f));

  ImGui::BeginDisabled();
  float v = 0.0f;
  ImGui::VSliderFloat("##v", size, &v, 0.0f, 1.0f, "", ImGuiSliderFlags_NoInput);
  ImGui::EndDisabled();

  ImGui::PopStyleColor(6);
  ImGui::PopStyleVar();

  ImGui::SetItemTooltip("No intent assigned to slot %d", slotIndex + 1);
}

void Gui::drawIntentControls() {
  ImGui::SeparatorText("Intents");
  drawIntentSlotSliders();

  // Collapsible editor for tuning intent characteristics (collapsed by default)
  if (ImGui::CollapsingHeader("Intent Characteristics")) {
    drawIntentCharacteristicsEditor();
  }
}

// Helper: convert influence (0..1) to color gradient green → amber → red
static ImU32 influenceToColorU32(float influence) {
  influence = ofClamp(influence, 0.0f, 1.0f);
  if (influence < 0.5f) {
    float t = influence * 2.0f; // 0..1 for first half
    return IM_COL32(
      static_cast<int>(ofLerp(100, 255, t)),   // R: 100→255
      static_cast<int>(ofLerp(200, 180, t)),   // G: 200→180
      static_cast<int>(ofLerp(100, 50, t)),    // B: 100→50
      255);
  } else {
    float t = (influence - 0.5f) * 2.0f; // 0..1 for second half
    return IM_COL32(
      255,                                      // R: stays 255
      static_cast<int>(ofLerp(180, 80, t)),    // G: 180→80
      static_cast<int>(ofLerp(50, 80, t)),     // B: 50→80
      255);
  }
}

void Gui::drawIntentCharacteristicsEditor() {
  float intentStrength = synthPtr->intentController->getStrength();
  constexpr float sliderWidth = 150.0f;
  constexpr float activationThreshold = 0.001f;
  
  const auto& intentActivations = synthPtr->intentController->getActivations();
  
  // Count active intents
  int activeCount = 0;
  for (const auto& ia : intentActivations) {
    if (ia.activation > activationThreshold) activeCount++;
  }
  
  // Show blended values only when multiple intents are active
  if (activeCount > 1) {
    const auto& active = synthPtr->intentController->getActiveIntent();
    ImGui::TextColored(GREY_COLOR, "Blended: E:%.2f D:%.2f S:%.2f C:%.2f G:%.2f",
                       active.getEnergy(), active.getDensity(),
                       active.getStructure(), active.getChaos(),
                       active.getGranularity());
    ImGui::Separator();
  }
  
  // Only show intents with activation > 0
  for (size_t i = 0; i < intentActivations.size(); ++i) {
    const auto& ia = intentActivations[i];
    if (ia.activation <= activationThreshold) continue;
    
    const Intent& intent = *ia.intentPtr;
    float influence = ia.activation * intentStrength;
    ImU32 indicatorColor = influenceToColorU32(influence);
    
    // Format tree node label with activation value
    char label[64];
    std::snprintf(label, sizeof(label), "%s (%.2f)", intent.getName().c_str(), ia.activation);
    
    // Draw influence indicator before tree node
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float indicatorSize = 8.0f;
    drawList->AddRectFilled(
      ImVec2(pos.x, pos.y + 4.0f),
      ImVec2(pos.x + indicatorSize, pos.y + 4.0f + indicatorSize),
      indicatorColor);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indicatorSize + 4.0f);
    
    // Use stable ID based on index so tree state persists when activation values change
    ImGui::PushID(static_cast<int>(i));
    if (ImGui::TreeNode("##intent", "%s", label)) {
      ImGui::PushItemWidth(sliderWidth);
      
      // Render sliders for each characteristic
      for (auto& param : intent.getParameterGroup()) {
        if (param->type() == typeid(ofParameter<float>).name()) {
          auto& fp = param->cast<float>();
          float v = fp.get();
          if (ImGui::SliderFloat(fp.getName().c_str(), &v, fp.getMin(), fp.getMax(), "%.2f")) {
            fp.set(v);
          }
        }
      }
      
      ImGui::PopItemWidth();
      ImGui::TreePop();
    }
    ImGui::PopID();
  }
}

void Gui::drawLayerControls() {
  ImGui::SeparatorText("Layers");
  
  // Build tooltip map from layer name to description (if available)
  std::unordered_map<std::string, std::string> layerTooltips;
  for (const auto& [name, layerPtr] : synthPtr->getDrawingLayers()) {
    if (!layerPtr || !layerPtr->isDrawn) continue;
    if (!layerPtr->description.empty()) {
      layerTooltips[layerPtr->name] = layerPtr->description;
    }
  }
  NodeRenderUtil::setLayerTooltipMap(&layerTooltips);
  drawVerticalSliders(synthPtr->getLayerAlphaParameters(), synthPtr->layerController->getPauseParamPtrs());
  NodeRenderUtil::setLayerTooltipMap(nullptr);
}

void Gui::drawDisplayControls() {
  // Collapsible section - starts collapsed for live performance
  if (!ImGui::CollapsingHeader("Display")) return;

  ofxImGui::Settings settings = ofxImGui::Settings();
  
  const char* tonemapOptions[] = {
    "Linear (clamp)",
    "Reinhard",
    "Reinhard Extended",
    "ACES",
    "Filmic",
    "Exposure"
  };
  auto& dc = *synthPtr->displayController;
  int currentTonemap = dc.getToneMapType().get();
  ImGui::PushItemWidth(150.0f);
  if (ImGui::Combo("##tonemap", &currentTonemap, tonemapOptions, IM_ARRAYSIZE(tonemapOptions))) {
    dc.getToneMapType().set(currentTonemap);
  }
  ImGui::PopItemWidth();
  ImGui::SameLine();
  ImGui::Text("%s", dc.getToneMapType().getName().c_str());

  addParameter(synthPtr, dc.getExposure());
  addParameter(synthPtr, dc.getGamma());
  addParameter(synthPtr, dc.getWhitePoint());
  addParameter(synthPtr, dc.getContrast());
  addParameter(synthPtr, dc.getSaturation());
  addParameter(synthPtr, dc.getBrightness());
  addParameter(synthPtr, dc.getHueShift());
  addParameter(synthPtr, dc.getSideExposure());
}

constexpr float thumbW = 128.0f;
constexpr ImVec2 thumbSize(thumbW, thumbW);

void Gui::drawInternalState() {
  if (synthPtr->liveTexturePtrFns.size() == 0) return;
  
  ImGui::SeparatorText("State");
  
  // Calculate total width needed for horizontal scrolling
  const float itemWidth = thumbW + 8.0f;
  const float totalWidth = itemWidth * synthPtr->liveTexturePtrFns.size();
  const float innerHeight = ImGui::GetTextLineHeightWithSpacing() + thumbW;
  const float panelHeight = innerHeight + ImGui::GetStyle().ScrollbarSize + 4.0f;
  
  // Set exact content size to prevent any vertical overflow
  ImGui::SetNextWindowContentSize(ImVec2(totalWidth, innerHeight));
  ImGui::BeginChild("tex_scroll", ImVec2(0, panelHeight), true, ImGuiWindowFlags_HorizontalScrollbar);

  constexpr float spacing = 8.0f;
  int index = 0;
  for (const auto& tex : synthPtr->liveTexturePtrFns) {
    if (index > 0) {
      ImGui::SameLine(0, spacing);
    }
    
    ImGui::BeginGroup();
    {
      ImGui::Text("%s", tex.first.c_str());
      const ofTexture* texturePtr = tex.second();
      if (!texturePtr || !texturePtr->isAllocated()) {
        ImGui::Dummy(ImVec2(thumbW, thumbW));
      } else {
        const auto& textureData = texturePtr->getTextureData();
        assert(textureData.textureTarget == GL_TEXTURE_2D);
        ImTextureID imguiTexId = (ImTextureID)(uintptr_t)textureData.textureID;
        ImGui::PushID(tex.first.c_str());
        ImGui::Image(imguiTexId, thumbSize);
        ImGui::PopID();
      }
    }
    ImGui::EndGroup();
    
    ++index;
  }

  ImGui::EndChild();
}

void Gui::drawMemoryBank() {
  if (!ImGui::CollapsingHeader("Memories", ImGuiTreeNodeFlags_DefaultOpen)) return;
  
  constexpr float memThumbW = 64.0f;
  constexpr float spacing = 4.0f;
  constexpr float slotHeight = 100.0f; // thumbnail + label + button
  
  // Scrollable horizontal region for thumbnails (no vertical scroll)
  ImGui::BeginChild("MemoryBankSlots", ImVec2(0, slotHeight), false, 
      ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollbar);
  
  // Horizontal layout of slots
  for (int i = 0; i < MemoryBank::NUM_SLOTS; i++) {
    ImGui::PushID(i);
    
    ImGui::BeginGroup();
    {
      // Thumbnail or empty box
      const ofTexture* tex = synthPtr->getMemoryBankController().getMemoryBank().get(i);
      if (tex && tex->isAllocated()) {
        const auto& textureData = tex->getTextureData();
        ImTextureID imguiTexId = (ImTextureID)(uintptr_t)textureData.textureID;
        ImGui::Image(imguiTexId, ImVec2(memThumbW, memThumbW));
        
        // Tooltip with larger preview on hover
        if (ImGui::IsItemHovered()) {
          ImGui::BeginTooltip();
          constexpr float tooltipSize = 256.0f;
          ImGui::Image(imguiTexId, ImVec2(tooltipSize, tooltipSize));

          MemoryBankController::AutoCaptureSlotDebug dbg;
          if (synthPtr->getMemoryBankController().getAutoCaptureSlotDebug(i, synthPtr->getSynthRunningTime(), dbg)) {
            const char* band = (dbg.band == 0) ? "long" : (dbg.band == 1) ? "mid" : "recent";
            ImGui::Separator();
            if (dbg.isAnchorLocked) {
              ImGui::Text("slot %d (%s) [anchor]", i, band);
            } else {
              ImGui::Text("slot %d (%s)", i, band);
            }

            if (dbg.captureTimeSec >= 0.0f) {
              float age = synthPtr->getSynthRunningTime() - dbg.captureTimeSec;
              ImGui::Text("age: %.1fs", age);
            } else {
              ImGui::TextUnformatted("age: --");
            }

            if (dbg.qualityScore >= 0.0f) {
              ImGui::Text("quality: %.6f", dbg.qualityScore);
            } else {
              ImGui::TextUnformatted("quality: --");
            }

            if (dbg.variance >= 0.0f && dbg.activeFraction >= 0.0f) {
              ImGui::Text("var: %.6f  active: %.3f", dbg.variance, dbg.activeFraction);
            }

            if (dbg.nextDueTimeSec >= 0.0f) {
              float untilDue = dbg.nextDueTimeSec - synthPtr->getSynthRunningTime();
              if (untilDue < 0.0f) {
                ImGui::Text("overdue: %.1fs", -untilDue);
              } else {
                ImGui::Text("next due: %.1fs", untilDue);
              }
            }
          }

          ImGui::EndTooltip();
        }
      } else {
        // Draw empty placeholder
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddRect(
            p, ImVec2(p.x + memThumbW, p.y + memThumbW),
            IM_COL32(128, 128, 128, 128));
        ImGui::Dummy(ImVec2(memThumbW, memThumbW));
      }
      
      // Save button - deferred to avoid GL state issues during ImGui rendering
      const std::string saveLabel = "Save " + std::to_string(i);
      if (ImGui::Button(saveLabel.c_str(), ImVec2(memThumbW, 0))) {
        synthPtr->getMemoryBankController().getMemoryBank().requestSaveToSlot(i);
      }
    }
    ImGui::EndGroup();
    
    ImGui::PopID();
    
    if (i < MemoryBank::NUM_SLOTS - 1) {
      ImGui::SameLine(0, spacing);
    }
  }
  
  // Place "Clear All" at far right of the scrolling list, vertically centered
  ImGui::SameLine(0, spacing * 4.0f);
  ImGui::BeginGroup();
  {
    const float buttonHeight = ImGui::GetFrameHeight();
    const float buttonsHeight = buttonHeight * 2.0f + ImGui::GetStyle().ItemSpacing.y;
    const float topPad = (slotHeight - buttonsHeight) * 0.5f;
    if (topPad > 0.0f) {
      ImGui::Dummy(ImVec2(0.0f, topPad));
    }

    if (ImGui::Button("Clear All", ImVec2(memThumbW, 0))) {
      synthPtr->getMemoryBankController().getMemoryBank().clearAll();
    }

    ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y));

    if (ImGui::Button("Save All", ImVec2(memThumbW, 0))) {
      synthPtr->requestSaveAllMemories();
    }
  }
  ImGui::EndGroup();
  
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
  
  // Time display: Clock | Synth | Config
  // Clock: wall time since first H key (never pauses)
  // Synth: accumulated running time (pauses when synth pauses)
  // Config: accumulated time in current config (resets on config load)
  
  int clockSec = static_cast<int>(synthPtr->getClockTimeSinceFirstRun());
  int clockMin = clockSec / 60;
  clockSec = clockSec % 60;
  
  int synthSec = static_cast<int>(synthPtr->getSynthRunningTime());
  int synthMin = synthSec / 60;
  synthSec = synthSec % 60;
  
  int configSec = static_cast<int>(synthPtr->getConfigRunningTime());
  int configMin = configSec / 60;
  configSec = configSec % 60;
  
  auto& nav = synthPtr->performanceNavigator;
  
  // Show config timer with countdown if duration is configured
  if (nav.hasConfigDuration()) {
    int countdownMin = nav.getCountdownMinutes();
    int countdownSec = nav.getCountdownSeconds();
    const char* sign = nav.isCountdownNegative() ? "-" : "";
    
    // Flash red when countdown is expired (toggle every 0.5 seconds)
    if (nav.isCountdownExpired()) {
      bool flash = static_cast<int>(ofGetElapsedTimef() * 2) % 2 == 0;
      if (flash) {
        ImGui::TextColored(RED_COLOR, "%02d:%02d | S %02d:%02d | C %02d:%02d / %s%02d:%02d", 
                           clockMin, clockSec, synthMin, synthSec, configMin, configSec, sign, countdownMin, countdownSec);
      } else {
        ImGui::Text("%02d:%02d | S %02d:%02d | C %02d:%02d / %s%02d:%02d", 
                    clockMin, clockSec, synthMin, synthSec, configMin, configSec, sign, countdownMin, countdownSec);
      }
    } else {
      ImGui::Text("%02d:%02d | S %02d:%02d | C %02d:%02d / %s%02d:%02d", 
                  clockMin, clockSec, synthMin, synthSec, configMin, configSec, sign, countdownMin, countdownSec);
    }
  } else {
    ImGui::Text("%02d:%02d | S %02d:%02d | C %02d:%02d", clockMin, clockSec, synthMin, synthSec, configMin, configSec);
  }
  
  // FPS counter on same line with spacing
  ImGui::SameLine(0.0f, 20.0f);  // 20 pixels spacing
  ImGui::Text("%s FPS", ofToString(ofGetFrameRate(), 0).c_str());
  
  // Status indicator: hibernation state takes priority over pause state
  auto hibernationState = synthPtr->getHibernationState();
  if (hibernationState == HibernationController::State::HIBERNATED) {
    ImGui::TextColored(YELLOW_COLOR, "Hibernated");
  } else if (hibernationState == HibernationController::State::FADING_OUT) {
    ImGui::TextColored(YELLOW_COLOR, "Hibernating...");
  } else if (synthPtr->paused) {
    ImGui::TextColored(YELLOW_COLOR, "%s Paused", PAUSE_ICON);
  } else {
    ImGui::TextColored(GREY_COLOR, "%s Playing", PLAY_ICON);
  }
  
#ifdef TARGET_MAC
  if (synthPtr->isRecording()) {
    ImGui::TextColored(RED_COLOR, "%s Recording", RECORD_ICON);
  } else {
    ImGui::TextColored(GREY_COLOR, "   Not Recording");
  }
#endif
  
  int saveCount = synthPtr->getActiveSaveCount();
  if (saveCount < 1) {
    ImGui::TextColored(GREY_COLOR, "   No Image Saves");
  } else {
    ImGui::TextColored(YELLOW_COLOR, "%s %d Image Save%s", SAVE_ICON, saveCount, saveCount > 1 ? "s" : "");
  }
}

constexpr int FBO_PARAMETER_ID = 0;
void Gui::drawNode(const ModPtr& modPtr, bool highlight) {
  int modId = modPtr->getId();
  
  // Check if any controller has received auto values (i.e., Mod responds to agency)
  bool hasReceivedAuto = false;
  for (const auto& [name, controllerPtr] : modPtr->sourceNameControllerPtrMap) {
    if (controllerPtr && controllerPtr->hasReceivedAutoValue) {
      hasReceivedAuto = true;
      break;
    }
  }
  float agency = modPtr->getAgency();
  bool isAgencyActive = agency > 0.0f && hasReceivedAuto;
  
  // Apply title bar color based on state
  int colorStylesPushed = 0;
  if (highlight) {
    ImNodes::PushColorStyle(ImNodesCol_TitleBar, IM_COL32(50, 200, 100, 255));
    ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, IM_COL32(70, 220, 120, 255));
    ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, IM_COL32(90, 240, 140, 255));
    colorStylesPushed = 3;
  } else if (isAgencyActive) {
    // Blue-purple tint for agency-active nodes (veering towards red)
    ImNodes::PushColorStyle(ImNodesCol_TitleBar, IM_COL32(70, 50, 120, 255));
    ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, IM_COL32(90, 60, 140, 255));
    ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, IM_COL32(110, 70, 160, 255));
    colorStylesPushed = 3;
  }
  
  ImNodes::BeginNode(modId);

  ImNodes::BeginNodeTitleBar();
  ImGui::TextUnformatted(modPtr->getName().c_str());
  
  if (isAgencyActive) {
    ImGui::ProgressBar(agency, ImVec2(64.0f, 4.0f), "");
  } else {
    // Subtle placeholder that blends with the title bar
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(35, 50, 70, 100));
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, IM_COL32(35, 50, 70, 100));
    ImGui::ProgressBar(0.0f, ImVec2(64.0f, 4.0f), "");
    ImGui::PopStyleColor(2);
  }
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
  
  // Pop title bar color styles
  for (int i = 0; i < colorStylesPushed; ++i) {
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
  if (!synthPtr || (synthPtr->modPtrs.empty() && synthPtr->getDrawingLayers().empty())) {
    ImGui::Begin("NodeEditor");
    ImGui::TextUnformatted("No synth configuration loaded.");
    ImGui::End();
    return;
  }

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
        layoutNeedsSave = false;  // Reset dirty flag after load
        ofLogNotice("Gui") << "Auto-loaded node layout for: " << synthPtr->name;
      }
    } else {
      // No stored layout: generate a deterministic layout immediately and persist it.
      // This keeps the node editor usable on first load without manual intervention.
      nodeEditorModel.relaxLayout(120);
      layoutComputed = true;
      animateLayout = false;
      layoutNeedsSave = false;

      if (nodeEditorModel.saveLayout()) {
        ofLogNotice("Gui") << "Auto-generated and saved node layout for: " << synthPtr->name;
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
    nodeEditorModel.randomizeLayout();
    layoutComputed = false;
    animateLayout = true;
  }
  
  // Run animated layout if enabled and not yet computed
  if (animateLayout && !layoutComputed) {
    nodeEditorModel.computeLayoutAnimated();
    if (!nodeEditorModel.isLayoutAnimating()) {
      layoutComputed = true; // Animation finished
      // Mark layout as needing save after animation completes
      nodeEditorModel.snapshotPositions();
      layoutNeedsSave = true;
      layoutChangeTime = ofGetElapsedTimef();
    }
  }
  
  ImGui::SameLine();
  ImGui::Text("|");
  ImGui::SameLine();
  drawSnapshotControls();
  
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
          bool isCurrent = currentLayerPtr && currentLayerPtr->get()->id == layerPtr->id;
          bool isConnectedToSelection = ImNodes::IsNodeSelected(sourceModId) || ImNodes::IsNodeSelected(layerNodeId);
          if (isCurrent || isConnectedToSelection) alpha = 255;
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
  
  // Check for layout changes on mouse release (only when interacting with node editor)
  // This avoids per-frame comparison overhead during performance
  if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    if (nodeEditorModel.hasPositionsChanged()) {
      layoutNeedsSave = true;
      layoutChangeTime = ofGetElapsedTimef();
    }
  }
  
  // Check if any Mod parameter was modified via GUI this frame (only if auto-save enabled)
  if (autoSaveModsEnabled && NodeRenderUtil::wasAnyParameterModified()) {
    modsConfigNeedsSave = true;
    modsConfigChangeTime = ofGetElapsedTimef();
  }
  NodeRenderUtil::resetModifiedFlag();
  
  // Debounced auto-save for layout
  if (layoutNeedsSave) {
    float elapsed = ofGetElapsedTimef() - layoutChangeTime;
    if (elapsed >= AUTO_SAVE_DELAY) {
      if (nodeEditorModel.saveLayout()) {
        ofLogNotice("Gui") << "Auto-saved node layout for: " << synthPtr->name;
      }
      layoutNeedsSave = false;
    }
  }
  
  // Debounced auto-save for mods config (only if auto-save enabled)
  if (autoSaveModsEnabled && modsConfigNeedsSave) {
    float elapsed = ofGetElapsedTimef() - modsConfigChangeTime;
    if (elapsed >= AUTO_SAVE_DELAY) {
      if (synthPtr->saveModsToCurrentConfig()) {
        ofLogNotice("Gui") << "Auto-saved mods config to: " << synthPtr->currentConfigPath;
      }
      modsConfigNeedsSave = false;
    }
  }
  
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

void Gui::drawPerformanceNavigator() {
  auto& nav = synthPtr->performanceNavigator;
  
  if (!nav.hasConfigs()) {
    ImGui::TextColored(GREY_COLOR, "No performance configs loaded");
    return;
  }
  
  ImGui::SeparatorText("Performance");
  
  // Hibernation fade indicator (fixed height, shows fade-out or fade-in progress)
  {
    const float barHeight = 4.0f;
    const ImVec2 barSize(-1, barHeight);
    const auto state = synthPtr->getHibernationState();
    const bool fadingOut = (state == HibernationController::State::FADING_OUT);
    const bool fadingIn = (state == HibernationController::State::FADING_IN);

    if (fadingOut) {
      // Alpha goes from 1.0 to 0.0, so progress = 1.0 - alpha
      float progress = 1.0f - synthPtr->hibernationController->getAlpha();
      ImGui::ProgressBar(progress, barSize, "");
    } else if (fadingIn) {
      // Alpha goes from 0.0 to 1.0, so progress = alpha (show wake progress)
      // Use a different color to distinguish from fade-out
      ImGui::PushStyleColor(ImGuiCol_PlotHistogram, IM_COL32(100,180,100,255));
      float progress = synthPtr->hibernationController->getAlpha();
      ImGui::ProgressBar(progress, barSize, "");
      ImGui::PopStyleColor();
    } else {
      ImGui::PushStyleColor(ImGuiCol_PlotHistogram, IM_COL32(130,130,130,140));
      ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(60,60,60,80));
      ImGui::ProgressBar(0.0f, barSize, "");
      ImGui::PopStyleColor(2);
    }
  }
  
  int currentIndex = nav.getCurrentIndex();
  int configCount = nav.getConfigCount();
  bool canPrev = currentIndex > 0;
  bool canNext = currentIndex < configCount - 1;

  // Place prev/next to the right of the grid (vertical space is precious).
  constexpr float buttonSize = 60.0f;
  constexpr float navGapY = 14.0f;

  float gridH = 0.0f;

  if (ImGui::BeginTable("##perf_grid_layout", 2,
                        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_NoBordersInBody)) {
    ImGui::TableSetupColumn("##grid", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("##nav", ImGuiTableColumnFlags_WidthFixed, buttonSize + 4.0f);
    ImGui::TableNextRow();

    // 8x7 config pad grid (y=0 is top row)
    ImGui::TableSetColumnIndex(0);
    {
      constexpr float kGap = 4.0f;
      constexpr float kMidExtraGap = 6.0f; // between x=3|4 and y=3|4
      constexpr float kMinPad = 14.0f;
      constexpr float kMaxPad = 28.0f;
      constexpr float kDimFactor = 0.20f;

      const float availW = ImGui::GetContentRegionAvail().x;
      float padSize = std::floor((availW - (PerformanceNavigator::GRID_WIDTH - 1) * kGap - kMidExtraGap) /
                                 PerformanceNavigator::GRID_WIDTH);
      padSize = ofClamp(padSize, kMinPad, kMaxPad);

      const float gridW = PerformanceNavigator::GRID_WIDTH * padSize + (PerformanceNavigator::GRID_WIDTH - 1) * kGap + kMidExtraGap;
      gridH = PerformanceNavigator::GRID_HEIGHT * padSize + (PerformanceNavigator::GRID_HEIGHT - 1) * kGap + kMidExtraGap;

      auto scaleColor = [](PerformanceNavigator::RgbColor c, float factor) -> PerformanceNavigator::RgbColor {
        auto clamp255 = [](float v) -> uint8_t {
          if (v < 0.0f) return 0;
          if (v > 255.0f) return 255;
          return static_cast<uint8_t>(v);
        };

        return {
          clamp255(static_cast<float>(c.r) * factor),
          clamp255(static_cast<float>(c.g) * factor),
          clamp255(static_cast<float>(c.b) * factor),
        };
      };

      auto toU32 = [](PerformanceNavigator::RgbColor c, uint8_t a = 255) -> ImU32 {
        return IM_COL32(c.r, c.g, c.b, a);
      };

      ImDrawList* drawList = ImGui::GetWindowDrawList();
      const ImVec2 start = ImGui::GetCursorScreenPos();

      for (int y = 0; y < PerformanceNavigator::GRID_HEIGHT; ++y) {
        for (int x = 0; x < PerformanceNavigator::GRID_WIDTH; ++x) {
          const float extraX = (x >= 4) ? kMidExtraGap : 0.0f;
          const float extraY = (y >= 4) ? kMidExtraGap : 0.0f;

          const ImVec2 pos(start.x + x * (padSize + kGap) + extraX,
                           start.y + y * (padSize + kGap) + extraY);
          const ImVec2 pos2(pos.x + padSize, pos.y + padSize);

          const int configIdx = nav.getGridConfigIndex(x, y);
          const bool isCurrent = (configIdx >= 0 && configIdx == currentIndex);
          const bool isJumpTarget = (configIdx >= 0 && nav.getActiveHold() == PerformanceNavigator::HoldAction::JUMP &&
                                     nav.getJumpTargetIndex() == configIdx);

          // Simulate "LED off" for empty/unassigned slots.
          PerformanceNavigator::RgbColor base = { 0, 0, 0 };
          if (configIdx >= 0) {
            base = nav.getConfigGridColor(configIdx);
            if (!isCurrent) {
              base = scaleColor(base, kDimFactor);
            }
          }

          const PerformanceNavigator::RgbColor holdAmber = { 255, 140, 0 };
          const PerformanceNavigator::RgbColor fill = isJumpTarget ? holdAmber : base;

          drawList->AddRectFilled(pos, pos2, toU32(fill), 2.0f);

          ImU32 borderColor = IM_COL32(60, 60, 60, 200);
          float borderThickness = 1.0f;

          if (configIdx >= 0) {
            borderColor = IM_COL32(90, 90, 90, 220);
          }

          if (isCurrent) {
            borderColor = IM_COL32(240, 240, 240, 255);
            borderThickness = 2.0f;
          } else if (isJumpTarget) {
            borderColor = IM_COL32(100, 200, 100, 255);
            borderThickness = 2.0f;
          }

          drawList->AddRect(pos, pos2, borderColor, 2.0f, 0, borderThickness);

          if (isJumpTarget) {
            float progress = nav.getHoldProgress();
            float w = padSize * progress;
            drawList->AddRectFilled(pos, ImVec2(pos.x + w, pos.y + padSize), IM_COL32(100, 200, 100, 80), 2.0f);
          }

          ImGui::SetCursorScreenPos(pos);
          ImGui::PushID(y * PerformanceNavigator::GRID_WIDTH + x);
          ImGui::InvisibleButton("##pad", ImVec2(padSize, padSize));
          ImGui::PopID();

          if (configIdx >= 0 && ImGui::IsItemHovered()) {
            const std::string configName = nav.getConfigName(configIdx);
            const std::string description = nav.getConfigDescription(configIdx);

            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 28.0f);
            ImGui::TextUnformatted(configName.c_str());
            if (!description.empty()) {
              ImGui::Separator();
              ImGui::TextUnformatted(description.c_str());
            }

            const ofTexture* thumb = nav.getConfigThumbnail(configIdx);
            if (thumb && thumb->isAllocated()) {
              ImGui::Separator();
              const auto& textureData = thumb->getTextureData();
              ImTextureID imguiTexId = (ImTextureID)(uintptr_t)textureData.textureID;

              constexpr float kMaxPreviewPx = 256.0f;
              float w = thumb->getWidth();
              float h = thumb->getHeight();
              if (w < 1.0f) w = 1.0f;
              if (h < 1.0f) h = 1.0f;

              float scale = 1.0f;
              float scaleW = kMaxPreviewPx / w;
              float scaleH = kMaxPreviewPx / h;
              scale = (scaleW < scaleH) ? scaleW : scaleH;
              if (scale > 1.0f) scale = 1.0f;

              ImGui::Image(imguiTexId, ImVec2(w * scale, h * scale));

              // Add a visible border so thumbnails don't blend into the tooltip background.
              {
                ImDrawList* tooltipDrawList = ImGui::GetWindowDrawList();
                const ImVec2 p0 = ImGui::GetItemRectMin();
                const ImVec2 p1 = ImGui::GetItemRectMax();
                tooltipDrawList->AddRect(p0, p1, IM_COL32(255, 255, 255, 255), 0.0f, 0, 2.0f);
              }
            }

            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
          }

          // Hold-to-confirm jump (mouse).
          if (configIdx >= 0 && !isCurrent) {
            if (ImGui::IsItemActive()) {
              if (nav.getActiveHold() != PerformanceNavigator::HoldAction::JUMP ||
                  nav.getJumpTargetIndex() != configIdx ||
                  nav.getActiveHoldSource() != PerformanceNavigator::HoldSource::MOUSE) {
                nav.beginHold(PerformanceNavigator::HoldAction::JUMP, PerformanceNavigator::HoldSource::MOUSE, configIdx);
              }
            } else if (!ImGui::IsItemActive() &&
                       nav.getActiveHold() == PerformanceNavigator::HoldAction::JUMP &&
                       nav.getJumpTargetIndex() == configIdx &&
                       nav.getActiveHoldSource() == PerformanceNavigator::HoldSource::MOUSE) {
              nav.endHold(PerformanceNavigator::HoldSource::MOUSE);
            }
          }
        }
      }

      // Advance cursor past manually positioned grid.
      ImGui::SetCursorScreenPos(ImVec2(start.x, start.y));
      ImGui::Dummy(ImVec2(gridW, gridH));
    }

    // PREV / NEXT buttons
    ImGui::TableSetColumnIndex(1);
    {
      const float navH = buttonSize * 2.0f + navGapY;
      float yPad = (gridH > navH) ? (gridH - navH) * 0.5f : 0.0f;
      if (yPad > 0.0f) {
        ImGui::Dummy(ImVec2(0.0f, yPad));
      }

      drawNavigationButton("##prev", -1, canPrev, buttonSize);
      ImGui::Dummy(ImVec2(0.0f, navGapY));
      drawNavigationButton("##next", +1, canNext, buttonSize);
    }

    ImGui::EndTable();
  }
}

void Gui::drawNavigationButton(const char* id, int direction, bool canNavigate, float buttonSize) {
  auto& nav = synthPtr->performanceNavigator;
  auto holdAction = (direction < 0) ? PerformanceNavigator::HoldAction::PREV 
                                    : PerformanceNavigator::HoldAction::NEXT;
  
  ImVec2 pos = ImGui::GetCursorScreenPos();
  ImVec2 center(pos.x + buttonSize / 2, pos.y + buttonSize / 2);
  float radius = buttonSize / 2 - 5;
  
  ImDrawList* drawList = ImGui::GetWindowDrawList();
  
  // Background circle
  if (canNavigate) {
    drawList->AddCircle(center, radius, IM_COL32(100, 100, 100, 255), 32, 2.0f);
  } else {
    drawList->AddCircle(center, radius, IM_COL32(60, 60, 60, 128), 32, 2.0f);
  }
  
  // Progress arc if holding this action
  if (nav.getActiveHold() == holdAction) {
    float progress = nav.getHoldProgress();
    float startAngle = -IM_PI / 2;
    float endAngle = startAngle + (progress * 2 * IM_PI);
    drawList->PathArcTo(center, radius, startAngle, endAngle, 32);
    drawList->PathStroke(IM_COL32(100, 200, 100, 255), false, 4.0f);
  }
  
  // Invisible button for interaction
  ImGui::InvisibleButton(id, ImVec2(buttonSize, buttonSize));
  
  // Draw arrow icon (direction determines which way it points)
  float arrowSize = radius * 0.5f;
  ImU32 arrowColor = canNavigate ? IM_COL32(255, 255, 255, 255) : IM_COL32(128, 128, 128, 128);
  if (direction < 0) {
    // Left arrow
    drawList->AddTriangleFilled(
      ImVec2(center.x - arrowSize * 0.5f, center.y),
      ImVec2(center.x + arrowSize * 0.5f, center.y - arrowSize * 0.6f),
      ImVec2(center.x + arrowSize * 0.5f, center.y + arrowSize * 0.6f),
      arrowColor);
  } else {
    // Right arrow
    drawList->AddTriangleFilled(
      ImVec2(center.x + arrowSize * 0.5f, center.y),
      ImVec2(center.x - arrowSize * 0.5f, center.y - arrowSize * 0.6f),
      ImVec2(center.x - arrowSize * 0.5f, center.y + arrowSize * 0.6f),
      arrowColor);
  }
  
  // Handle mouse hold
  if (ImGui::IsItemActive() && canNavigate) {
    if (nav.getActiveHold() != holdAction ||
        nav.getActiveHoldSource() != PerformanceNavigator::HoldSource::MOUSE) {
      nav.beginHold(holdAction, PerformanceNavigator::HoldSource::MOUSE);
    }
  } else if (!ImGui::IsItemActive() && nav.getActiveHold() == holdAction &&
             nav.getActiveHoldSource() == PerformanceNavigator::HoldSource::MOUSE) {
    nav.endHold(PerformanceNavigator::HoldSource::MOUSE);
  }
  
  // Tooltip
  if (ImGui::IsItemHovered() && canNavigate) {
    const char* dirLabel = (direction < 0) ? "previous" : "next";
    ImGui::SetTooltip("Hold to go to %s config\n(or use arrow keys)", dirLabel);
  }
}

bool Gui::loadSnapshotSlot(int slotIndex) {
  if (!synthPtr) return false;

  if (!snapshotsLoaded) {
    snapshotManager.loadFromFile(synthPtr->getName());
    snapshotsLoaded = true;
  }

  if (slotIndex < 0 || slotIndex >= ModSnapshotManager::NUM_SLOTS) return false;
  if (!snapshotManager.isSlotOccupied(slotIndex)) return false;

  auto snapshot = snapshotManager.getSlot(slotIndex);
  if (!snapshot) return false;

  auto affected = snapshotManager.apply(synthPtr, *snapshot);
  highlightedMods = affected;
  highlightStartTime = ofGetElapsedTimef();
  autoSaveModsEnabled = false;  // Disable auto-save when loading snapshots
  ofLogNotice("Gui") << "Loaded snapshot from slot " << (slotIndex + 1);
  return true;
}

void Gui::drawSnapshotControls() {
  // Get selected Mods
  auto selectedMods = getSelectedMods();
  bool hasSelection = !selectedMods.empty();
  bool hasName = strlen(snapshotNameBuffer) > 0;
  
  // Snapshot slot buttons (inline with Random Layout)
  for (int i = 0; i < ModSnapshotManager::NUM_SLOTS; ++i) {
    if (i > 0) ImGui::SameLine();
    
    bool occupied = snapshotManager.isSlotOccupied(i);
    std::string label = std::to_string(i + 1);
    
    // Determine button action
    bool shiftHeld = ImGui::GetIO().KeyShift;
    int duplicateSlot = hasName ? snapshotManager.findNameInOtherSlot(snapshotNameBuffer, i) : -1;
    bool nameConflict = duplicateSlot >= 0;
    bool canSave = hasName && hasSelection && !nameConflict;
    bool canLoad = occupied && !hasName && !shiftHeld;
    bool canClear = occupied && shiftHeld;  // Shift+click to clear
    
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
          autoSaveModsEnabled = false;  // Disable auto-save when loading snapshots
          ofLogNotice("Gui") << "Loaded snapshot from slot " << (i + 1);
        }
      } else if (canClear) {
        // Clear slot (Shift+click)
        snapshotManager.clearSlot(i);
        snapshotManager.saveToFile(synthPtr->getName());
        ofLogNotice("Gui") << "Cleared snapshot slot " << (i + 1);
      }
    }
    ImGui::PopID();
    
    if (disabled) ImGui::EndDisabled();
    if (occupied) ImGui::PopStyleColor(3);
    
    // Tooltip (allow on disabled items too)
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
      if (nameConflict) {
        ImGui::SetTooltip("Name '%s' already in slot %d", snapshotNameBuffer, duplicateSlot + 1);
      } else if (occupied) {
        auto snapshot = snapshotManager.getSlot(i);
        if (snapshot) {
          if (hasName && hasSelection) {
            ImGui::SetTooltip("%s\n%zu mods\n\n[Click to save]\n[Shift+click to clear]",
                              snapshot->name.c_str(),
                              snapshot->modParams.size());
          } else if (hasName && !hasSelection) {
            ImGui::SetTooltip("%s\n%zu mods\n\n[Select mods to save]\n[Shift+click to clear]",
                              snapshot->name.c_str(),
                              snapshot->modParams.size());
          } else {
            ImGui::SetTooltip("%s\n%zu mods\n\n[Click to load]\n[Shift+click to clear]",
                              snapshot->name.c_str(),
                              snapshot->modParams.size());
          }
        }
      } else {
        if (hasName && hasSelection) {
          ImGui::SetTooltip("[Click to save]");
        } else if (hasName && !hasSelection) {
          ImGui::SetTooltip("[Select mods to save]");
        } else if (!hasName && hasSelection) {
          ImGui::SetTooltip("[Type name to save]");
        } else {
          ImGui::SetTooltip("[Select mods + type name to save]");
        }
      }
    }
  }
  
  // Text input for snapshot name
  ImGui::SameLine();

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
  
  // Auto-save mods toggle
  ImGui::SameLine();
  ImGui::Text("|");
  ImGui::SameLine();
  ImGui::Checkbox("auto-save configs", &autoSaveModsEnabled);
  
  // Keyboard shortcuts for loading slots (F1-F8 keys)
  for (int i = 0; i < ModSnapshotManager::NUM_SLOTS; ++i) {
    ImGuiKey key = static_cast<ImGuiKey>(ImGuiKey_F1 + i);
    if (ImGui::IsKeyPressed(key) && !ImGui::GetIO().WantTextInput) {
      if (synthPtr) {
        synthPtr->loadModSnapshotSlot(i);
      }
    }
  }
}

void Gui::drawHelpWindow() {
  if (!showHelpWindow) return;
  
  ImGui::SetNextWindowSize(ImVec2(420, 400), ImGuiCond_FirstUseEver);
  
  if (ImGui::Begin("Keyboard Shortcuts", &showHelpWindow)) {
    // Use monospace font if available
    if (monoFont) {
      ImGui::PushFont(monoFont);
    }
    
    ImGui::TextUnformatted(HELP_CONTENT);
    
    if (monoFont) {
      ImGui::PopFont();
    }
  }
  ImGui::End();
}

void Gui::drawDebugView() {
  if (!synthPtr->isDebugViewEnabled()) return;
  
  const ofFbo& fbo = synthPtr->getDebugViewFbo();
  if (!fbo.isAllocated()) return;
  
  // Set initial window size
  ImGui::SetNextWindowSize(ImVec2(520, 540), ImGuiCond_FirstUseEver);
  
  bool visible = true;
  ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
  if (ImGui::Begin("Debug View", &visible, flags)) {
    const auto& texData = fbo.getTexture().getTextureData();
    ImTextureID texId = (ImTextureID)(uintptr_t)texData.textureID;
    
    // Handle texture flipping (openFrameworks FBOs are typically flipped)
    ImVec2 uv0(0, texData.bFlipTexture ? 1 : 0);
    ImVec2 uv1(1, texData.bFlipTexture ? 0 : 1);
    
    // Scale image to fit available content area while maintaining aspect ratio
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float fboAspect = static_cast<float>(fbo.getWidth()) / static_cast<float>(fbo.getHeight());
    float availAspect = avail.x / avail.y;
    
    ImVec2 displaySize;
    if (availAspect > fboAspect) {
      // Window is wider than texture - fit to height
      displaySize = ImVec2(avail.y * fboAspect, avail.y);
    } else {
      // Window is taller than texture - fit to width
      displaySize = ImVec2(avail.x, avail.x / fboAspect);
    }
    
    ImGui::Image(texId, displaySize, uv0, uv1);
  }
  
  // Handle window close button
  if (!visible) {
    synthPtr->setDebugViewEnabled(false);
  }
  ImGui::End();
}



} // namespace ofxMarkSynth
