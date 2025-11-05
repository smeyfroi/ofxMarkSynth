//
//  Gui.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 05/11/2025.
//

#include "Gui.hpp"
#include "Synth.hpp"
#include "ImHelpers.h"



namespace ofxMarkSynth {



void Gui::setup(std::shared_ptr<Synth> synthPtr_, std::shared_ptr<ofAppBaseWindow> windowPtr) {
  synthPtr = synthPtr_;
  
  imgui.setup(windowPtr);

  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  // Keep Viewports disabled so everything stays inside this window:
  // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

  ImGuiStyle& style = ImGui::GetStyle();
  style.WindowRounding = 4.f;
  
//  gui.setup(parameters);
//  minimizeAllGuiGroupsRecursive(gui);
//
//  gui.add(activeIntentInfoLabel1.setup("I1", ""));
//  gui.add(activeIntentInfoLabel2.setup("I2", ""));
//  gui.add(pauseStatus.setup("Paused", ""));
//  gui.add(recorderStatus.setup("Recording", ""));
//  gui.add(saveStatus.setup("# Image Saves", ""));
}

void Gui::exit() {
  imgui.exit();
}

void Gui::draw() {
  auto mainSettings = ofxImGui::Settings();
  imgui.begin();
  
  //  if (ImGui::BeginMainMenuBar()) {
  //      if (ImGui::BeginMenu("File")) {
  //          if (ImGui::MenuItem("Quit")) ofExit();
  //          ImGui::EndMenu();
  //      }
  //      if (ImGui::BeginMenu("View")) {
  //          ImGui::MenuItem("Show Demo Window", nullptr, &showDemo);
  //          ImGui::EndMenu();
  //      }
  //      ImGui::EndMainMenuBar();
  //  }
  
  {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    
    ImGuiWindowFlags hostFlags = ImGuiWindowFlags_NoTitleBar |
                                 ImGuiWindowFlags_NoCollapse |
                                 ImGuiWindowFlags_NoResize   |
                                 ImGuiWindowFlags_NoMove     |
                                 ImGuiWindowFlags_NoBringToFrontOnFocus |
                                 ImGuiWindowFlags_NoNavFocus |
                                 ImGuiWindowFlags_NoDocking;       // prevent docking *into* this host
    ImGui::Begin("DockspaceWindow", nullptr, hostFlags);
    
    ImGuiID dockspace_id = ImGui::GetID("DockSpace");
    ImGuiDockNodeFlags dockFlags = 0;
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockFlags);
    
    if (ofxImGui::BeginWindow(synthPtr->name, mainSettings)) {
      ofxImGui::AddGroup(synthPtr->parameters, mainSettings);
      
      if (ofxImGui::BeginTree("Status", mainSettings)) {
        ImGui::Text("Paused: %s", synthPtr->paused ? "Yes" : "No");
        ImGui::Text("Recording: %s",
#ifdef TARGET_MAC
                    synthPtr->recorder.isRecording() ? "Yes" : "No"
#else
                    "N/A"
#endif
                    );
        ImGui::Text("# Image Saves: %d", SaveToFileThread::activeThreadCount);
        ofxImGui::EndTree(mainSettings);
      }
    }
    ofxImGui::EndWindow(mainSettings);
    
    
    ImGui::Begin("Inspector");
    ImGui::Text("Window size: %dx%d", ofGetWidth(), ofGetHeight());
    ImGui::Text("Window pos:  (%d, %d)", ofGetWindowPositionX(), ofGetWindowPositionY());
    ImGui::End();
    
    
    
    
    ImGui::End(); // DockspaceWindow
  }
  imgui.end();
  
  imgui.draw();
}



}
