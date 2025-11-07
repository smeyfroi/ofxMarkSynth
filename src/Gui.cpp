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
  
  auto mainSettings = ofxImGui::Settings();
  imgui.begin();
  
  drawDockspace();
  drawLog();
  drawSynthControls();
  drawModTree(mainSettings);
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

  ImGuiID dockspaceId = ImGui::GetID("MyDockSpace");
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
  
  ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left,  0.25f, &dockLeft, &dockMain);
  ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down,  0.15f, &dockBottom, &dockMain);
  ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.30f, &dockRight, &dockCenter);
  
  ImGui::DockBuilderDockWindow(synthPtr->parameters.getName().c_str(), dockLeft);
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

void Gui::addParameterGroup(ofParameterGroup& paramGroup) {
  for (size_t i = 0; i < paramGroup.size(); ++i) {
    auto& param = paramGroup[i];
    
    if (param.type() == typeid(ofParameterGroup).name()) {
      if (ImGui::TreeNode(param.getName().c_str())) {
        addParameterGroup(param.castGroup());
        ImGui::TreePop();
      }
    } else if (param.type() == typeid(ofParameter<int>).name()) {
      auto& intParam = param.cast<int>();
      addParameter(intParam);
    } else if (param.type() == typeid(ofParameter<float>).name()) {
      auto& floatParam = param.cast<float>();
      addParameter(floatParam);
    } else if (param.type() == typeid(ofParameter<ofFloatColor>).name()) {
      auto& colorParam = param.cast<ofFloatColor>();
      addParameter(colorParam);
    } else if (param.type() == typeid(ofParameter<glm::vec2>).name()) {
      auto& colorParam = param.cast<glm::vec2>();
      addParameter(colorParam);
    } else {
      ImGui::Text("Unsupported parameter type: %s", param.type().c_str());
    }
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

void Gui::drawModTree(ofxImGui::Settings settings) {
  TS_START("Gui::drawModTree");
  ImGui::Begin(synthPtr->parameters.getName().c_str());
  addParameterGroup(synthPtr->parameters);
  ImGui::End();
  TS_STOP("Gui::drawModTree");
}

void Gui::drawNodeEditor() {
  if (nodeEditorDirty) {
      nodeEditorModel.buildFromSynth(synthPtr);
      nodeEditorDirty = false;
  }

  ImGui::Begin("NodeEditor");
  ImNodes::BeginNodeEditor();
  
  for (const auto& node : nodeEditorModel.nodes) {
    ImNodes::BeginNode(node.nodeId);
    
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted(node.modPtr->name.c_str());
    ImNodes::EndNodeTitleBar();
    
//    // Input pins (sinks)
//    for (int pinId : node.inputPinIds) {
//      ImNodes::BeginInputAttribute(pinId);
//      // Display sink name from Mod
//      ImNodes::EndInputAttribute();
//    }
//    
//    // Output pins (sources)
//    for (int pinId : node.outputPinIds) {
//      ImNodes::BeginOutputAttribute(pinId);
//      // Display source name from Mod
//      ImNodes::EndOutputAttribute();
//    }
    ImGui::Dummy(ImVec2(10.0f, 0.0f));

    ImNodes::EndNode();
  }
  
  ImNodes::EndNodeEditor();
  ImGui::End();
}




} // namespace ofxMarkSynth
