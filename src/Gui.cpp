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



namespace ofxMarkSynth {



void Gui::setup(std::shared_ptr<Synth> synthPtr_, std::shared_ptr<ofAppBaseWindow> windowPtr) {
  synthPtr = synthPtr_;
  
  imgui.setup(windowPtr);

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
  ImGui::DockBuilderDockWindow("Viewport", dockCenter);
  
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
  
  addParameterFloat(synthPtr->agencyParameter);
  
  ImGui::SeparatorText("Intents");
  drawIntentControls();
  
  ImGui::SeparatorText("Layers");
  drawLayerControls();
  
  ImGui::SeparatorText("Display");
  drawDisplayControls();
  
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
      
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() - xPad);
      ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + colW);
      ImGui::TextWrapped("%s", paramGroup[i].getName().substr(0, 3).c_str());
      ImGui::PopTextWrapPos();
      ImGui::EndGroup();
      
      ImGui::PopID();
    }
    
    ImGui::EndTable();
  }
  ImGui::PopStyleVar();
}

void Gui::addParameterFloat(ofParameter<float>& parameter) {
  float value = parameter.get();
  ImGui::Text("%s", parameter.getName().c_str());
  ImGui::SameLine();
  if (ImGui::SliderFloat(("##" + parameter.getName()).c_str(), &value, parameter.getMin(), parameter.getMax(), "%.2f")) {
    parameter.set(value);
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
  
  auto& bgColorParam = synthPtr->backgroundColorParameter;
  ofFloatColor bgColor = bgColorParam.get();
  float col[4] = { bgColor.r, bgColor.g, bgColor.b, bgColor.a };
  ImGui::Text("%s", bgColorParam.getName().c_str());
  if (ImGui::ColorEdit4("##bgColor", col)) {
    bgColorParam.set(ofFloatColor(col[0], col[1], col[2], col[3]));
  }
  
  addParameterFloat(synthPtr->backgroundMultiplierParameter);

  const char* tonemapOptions[] = {
    "Linear (clamp)",
    "Reinhard",
    "Reinhard Extended",
    "ACES",
    "Filmic",
    "Exposure"
  };
  int currentTonemap = synthPtr->toneMapTypeParameter.get();
  ImGui::Text("%s", synthPtr->toneMapTypeParameter.getName().c_str());
  ImGui::SameLine();
  ImGui::PushItemWidth(150.0f);
  if (ImGui::Combo("##tonemap", &currentTonemap, tonemapOptions, IM_ARRAYSIZE(tonemapOptions))) {
    synthPtr->displayParameters[2].cast<int>().set(currentTonemap);
  }
  ImGui::PopItemWidth();
  
  addParameterFloat(synthPtr->exposureParameter);
  addParameterFloat(synthPtr->gammaParameter);
  addParameterFloat(synthPtr->whitePointParameter);
  addParameterFloat(synthPtr->contrastParameter);
  addParameterFloat(synthPtr->saturationParameter);
  addParameterFloat(synthPtr->brightnessParameter);
  addParameterFloat(synthPtr->hueShiftParameter);
  addParameterFloat(synthPtr->sideExposureParameter);
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
  ofxImGui::AddGroup(synthPtr->parameters, settings); // name of the param group needs to match the dock window
  TS_STOP("Gui::drawModTree");
}



} // namespace ofxMarkSynth
