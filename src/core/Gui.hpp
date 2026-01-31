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
#include "nodeEditor/NodeEditorModel.hpp"
#include "config/ModSnapshotManager.hpp"
#include "controller/AudioInspectorModel.hpp"
#include <algorithm>
#include <array>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>



namespace ofxMarkSynth {



class Synth;
class Intent;
class AgencyControllerMod;



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
  void drawAgencyControls();
  void drawHelpWindow();

  void drawIntentControls();
  void drawIntentSlotSliders();
  void drawDisabledSlider(const ImVec2& size, int slotIndex);
  void drawIntentCharacteristicsEditor();
  void drawIntentActivationTooltip(int slotIndex, const Intent* intentPtr, float activationValue);
  void drawIntentPresetTooltip(int slotIndex, const Intent& intent, float activationValue);
  void drawIntentImpactComparisonGrid(int selectedSlotIndex);
  int getIntentImpactValue(const Intent& intent, const std::string& key) const;
  void drawImpactSwatch(int impact, float size, bool highlight) const;
  void drawIntentImpactMiniGrid(const Intent& intent);
  void drawLayerControls();
  void drawDisplayControls();
  void drawInternalState();
  void drawMemoryBank();
  void drawStatus();
  void drawAgencyControllerNodeTitleBar(AgencyControllerMod* agencyControllerPtr);
  void drawAgencyControllerNodeTooltip(AgencyControllerMod* agencyControllerPtr);
  void drawNode(const ModPtr& modPtr, bool highlight = false);
  void drawNode(const DrawingLayerPtr& layerPtr);
  void drawNodeEditor();
  void drawSnapshotControls();
  void drawPerformanceNavigator();
  void drawNavigationButton(const char* id, int direction, bool canNavigate, float buttonSize);
  void drawDebugView();
  void drawAudioInspector();
  void drawVideoInspector();
  std::vector<ModPtr> getSelectedMods();

  struct RingBuffer {
    static constexpr int MAX_SAMPLES = 120; // ~2s @ 60fps

    std::array<float, MAX_SAMPLES> values {};
    int head { 0 };
    int count { 0 };

    void clear() {
      head = 0;
      count = 0;
    }

    void push(float v) {
      values[head] = v;
      head = (head + 1) % MAX_SAMPLES;
      count = std::min(count + 1, MAX_SAMPLES);
    }

    float get(int i) const {
      if (count <= 0) return 0.0f;
      i = std::max(0, std::min(i, count - 1));
      const int start = (head - count + MAX_SAMPLES) % MAX_SAMPLES;
      const int idx = (start + i) % MAX_SAMPLES;
      return values[idx];
    }

    float getMax() const {
      float m = 0.0f;
      for (int i = 0; i < count; ++i) {
        m = std::max(m, get(i));
      }
      return m;
    }

    float getMean() const {
      if (count <= 0) return 0.0f;
      float sum = 0.0f;
      for (int i = 0; i < count; ++i) {
        sum += get(i);
      }
      return sum / static_cast<float>(count);
    }
  };

  struct MotionMagnitudePlotState {
    RingBuffer flowSpeedMax;
    RingBuffer outMax;

    float heldFlowSpeedMax { 0.0f };
    float heldOutMax { 0.0f };
    int heldSampleCount { 0 };
    float heldTimestamp { -1.0f };
    bool heldValid { false };
  };

  struct VideoSamplingPlotState {
    RingBuffer acceptedCount;
    RingBuffer attemptedCount;
    RingBuffer acceptedAny;
    RingBuffer acceptedSpeedMax;
    RingBuffer acceptRate;

    float heldAcceptedSpeedMean { 0.0f };
    float heldAcceptedSpeedMax { 0.0f };
    float heldAcceptRate { 0.0f };
    float heldTimestamp { -1.0f };
    bool heldValid { false };
  };

  std::shared_ptr<Synth> synthPtr;
  ofxImGui::Gui imgui;
  bool dockBuilt { false };

  NodeEditorModel nodeEditorModel;
  bool nodeEditorDirty { true };           // rebuild on next frame
  bool animateLayout { true };             // animate layout on load
  bool layoutComputed { false };           // track if layout has been computed
  bool layoutAutoLoadAttempted { false };  // track if we've tried auto-load

  // Auto-save layout with debounce
  bool layoutNeedsSave { false };    // layout has changed and needs saving
  float layoutChangeTime { 0.0f };   // time when layout change was detected

  // Auto-save mods config with debounce
  bool autoSaveModsEnabled { false };    // toggle for auto-saving mods config (OFF by default)
  bool modsConfigNeedsSave { false };    // mods config has changed and needs saving
  float modsConfigChangeTime { 0.0f };   // time when mods config change was detected

  static constexpr float AUTO_SAVE_DELAY { 1.0f };  // seconds to wait before auto-saving

  AudioInspectorModel audioInspectorModel;

  ModSnapshotManager snapshotManager;
  bool snapshotsLoaded { false };
  std::string snapshotsConfigId;
  char snapshotNameBuffer[64] = "";
  std::unordered_set<std::string> highlightedMods;  // Mods to highlight after load
  float highlightStartTime { 0.0f };
  static constexpr float HIGHLIGHT_DURATION { 1.5f };  // seconds

  VideoSamplingPlotState videoSamplingPlotState;
  std::unordered_map<std::string, MotionMagnitudePlotState> motionMagnitudePlotStates;

  // Help window
  bool showHelpWindow { false };
  ImFont* monoFont { nullptr };
};



} // namespace ofxMarkSynth
