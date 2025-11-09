//
//  Gui.hpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 05/11/2025.
//

#pragma once

#include "ofxImGui.h"
#include "ImHelpers.h"
#include "ofColor.h"
#include "NodeEditorModel.hpp"
#include <memory>



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
  
  void drawVerticalSliders(ofParameterGroup& paramGroup);
  void addParameter(const ModPtr& modPtr, ofParameter<int>& parameter);
  void addParameter(const ModPtr& modPtr, ofParameter<float>& parameter);
  void addParameter(const ModPtr& modPtr, ofParameter<ofFloatColor>& parameter);
  void addParameter(const ModPtr& modPtr, ofParameter<glm::vec2>& parameter);
  void addParameter(const ModPtr& modPtr, ofAbstractParameter& parameterPtr);
  void addParameterGroup(const ModPtr& modPtr, ofParameterGroup& paramGroup);
  void addContributionWeights(const ModPtr& modPtr, const std::string& paramName);

  void drawIntentControls();
  void drawLayerControls();
  void drawDisplayControls();
  void drawInternalState();
  void drawStatus();
  void drawNodeEditor();

  std::shared_ptr<Synth> synthPtr;
  ofxImGui::Gui imgui;
  bool dockBuilt { false };
  
  NodeEditorModel nodeEditorModel;
  bool nodeEditorDirty { true };      // rebuild on next frame
  bool animateLayout { true };        // animate layout on load
  bool layoutComputed { false };      // track if layout has been computed
  bool layoutAutoLoadAttempted { false }; // track if we've tried auto-load

};



} // ofxMarkSynth
