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
#include "util/ModSnapshotManager.hpp"
#include <memory>
#include <unordered_set>
#include <vector>



namespace ofxMarkSynth {



class Synth;



class Gui {
  
public:
  void setup(std::shared_ptr<Synth> synthPtr_, std::shared_ptr<ofAppBaseWindow> windowPtr);
  void exit();
  void draw();
  void toggleHelpWindow() { showHelpWindow = !showHelpWindow; }
  void onConfigLoaded(); // a Synth config is successfully loaded

  bool loadSnapshotSlot(int slotIndex);

private:
  void drawDockspace();
  void buildInitialDockLayout(ImGuiID dockspaceId);
  void drawLog();
  void drawSynthControls();
  void drawHelpWindow();
  
  void drawIntentControls();
  void drawIntentSlotSliders();
  void drawIntentCharacteristicsEditor();
  void drawLayerControls();
  void drawDisplayControls();
  void drawInternalState();
  void drawMemoryBank();
  void drawStatus();
  void drawNode(const ModPtr& modPtr, bool highlight = false);
  void drawNode(const DrawingLayerPtr& layerPtr);
  void drawNodeEditor();
  void drawSnapshotControls();
  void drawPerformanceNavigator();
  std::vector<ModPtr> getSelectedMods();

  std::shared_ptr<Synth> synthPtr;
  ofxImGui::Gui imgui;
  bool dockBuilt { false };
  
  NodeEditorModel nodeEditorModel;
  bool nodeEditorDirty { true };      // rebuild on next frame
  bool animateLayout { true };        // animate layout on load
  bool layoutComputed { false };      // track if layout has been computed
  bool layoutAutoLoadAttempted { false }; // track if we've tried auto-load

  // Snapshot system for saving/recalling Mod parameters
  ModSnapshotManager snapshotManager;
  bool snapshotsLoaded { false };
  char snapshotNameBuffer[64] = "";
  std::unordered_set<std::string> highlightedMods;  // Mods to highlight after load
  float highlightStartTime { 0.0f };
  static constexpr float HIGHLIGHT_DURATION { 1.5f };  // seconds
  
  // Help window
  bool showHelpWindow { false };
  ImFont* monoFont { nullptr };
};



} // ofxMarkSynth
