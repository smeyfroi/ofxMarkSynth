//
//  Gui.hpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 05/11/2025.
//

#pragma once

#include "ofxImGui.h"
#include "ImHelpers.h"



namespace ofxMarkSynth {



class Synth;



class Gui {
  
public:
  void setup(std::shared_ptr<Synth> synthPtr_, std::shared_ptr<ofAppBaseWindow> windowPtr);
  void exit();
  void draw();
  
  
  
private:
  void drawDockspace();
  void buildInitialDockLayout(ImGuiID dockspaceId);
  void drawLog();
  void drawSynthControls();
  void drawModTree(ofxImGui::Settings settings);
  
  void drawVerticalSliders(ofParameterGroup& paramGroup);
  void addParameter(ofParameter<float>& parameter);
  
  void drawIntentControls();
  void drawLayerControls();
  void drawDisplayControls();
  void drawStatus();

  std::shared_ptr<Synth> synthPtr;
  ofxImGui::Gui imgui;
  bool dockBuilt { false };
};



} // ofxMarkSynth
