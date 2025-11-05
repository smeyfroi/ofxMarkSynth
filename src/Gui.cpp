//
//  Gui.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 05/11/2025.
//

#include "Gui.hpp"
#include "Synth.hpp"
#include "ImHelpers.h"
#include "imgui_internal.h" // for DockBuilder



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
  auto mainSettings = ofxImGui::Settings();
  imgui.begin();
  
  drawDockspace();
  drawLog();
  drawStatus();
  
  imgui.end();
  imgui.draw();
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
  
  ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left,  0.20f, &dockLeft, &dockMain);
  ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down,  0.25f, &dockBottom, &dockMain);
  ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.22f, &dockRight, &dockCenter);
  
  ImGui::DockBuilderDockWindow("Mods", dockLeft);
  ImGui::DockBuilderDockWindow("Status", dockRight);
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
void Gui::drawStatus() {
  ImGui::Begin("Status");
  
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
  
  ImGui::End();
}
  
//    if (ofxImGui::BeginWindow(synthPtr->name, mainSettings)) {
//      ofxImGui::AddGroup(synthPtr->parameters, mainSettings);
//      
//    }
//    ofxImGui::EndWindow(mainSettings);



}
