//
//  Synth.hpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#pragma once

#include "core/Mod.hpp"
#include <array>
#include <filesystem>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "rendering/AsyncImageSaver.hpp"
#include "core/Intent.hpp"
#include "core/ParamController.h"
#include "core/Gui.hpp"
#include "nodeEditor/NodeEditorModel.hpp"
#include "nodeEditor/NodeEditorLayout.hpp"
#include "gui/LoggerChannel.hpp"
#include "config/ModFactory.hpp"
#include "config/PerformanceNavigator.hpp"
#include "controller/MemoryBankController.hpp"
#include "rendering/VideoRecorder.hpp"
#include "controller/HibernationController.hpp"
#include "controller/TimeTracker.hpp"
#include "controller/ConfigTransitionManager.hpp"
#include "controller/IntentController.hpp"
#include "controller/LayerController.hpp"
#include "controller/DisplayController.hpp"
#include "controller/CueGlyphController.hpp"
#include "rendering/CompositeRenderer.hpp"

namespace ofxAudioAnalysisClient {
class LocalGistClient;
}

namespace ofxMarkSynth {

const ofFloatColor DEFAULT_CLEAR_COLOR { 0.0, 0.0, 0.0, 0.0 };

using ModPtrMap = std::unordered_map<std::string, ofxMarkSynth::ModPtr>;

class Synth : public Mod {

public:
  // The composite is the middle (square) section, scaled to fit the window height
  static std::shared_ptr<Synth> create(const std::string& name, ModConfig config, bool startHibernated, glm::vec2 compositeSize, ResourceManager resources = {});

protected:
  Synth(const std::string& name,
        ModConfig config,
        bool startHibernated,
        glm::vec2 compositeSize_,
        std::shared_ptr<ofxAudioAnalysisClient::LocalGistClient> audioAnalysisClient,
        ResourceManager resources = {});

public:
  void drawGui();
  void shutdown() override;

  template <typename ModT>
  ofxMarkSynth::ModPtr addMod(const std::string& name, ofxMarkSynth::ModConfig&& modConfig);

  template <typename ModT, typename... Args>
  ofxMarkSynth::ModPtr addMod(const std::string& name, ofxMarkSynth::ModConfig&& modConfig, Args&&... args);

  ofxMarkSynth::ModPtr getMod(const std::string& name) const { return modPtrs.at(name); }
  const ModPtrMap& getMods() const { return modPtrs; }

  enum class DebugViewMode {
    Fbo,
    AudioInspector,
    VideoInspector,
  };

  DebugViewMode getDebugViewMode() const { return debugViewMode; }
  void setDebugViewMode(DebugViewMode mode) { debugViewMode = mode; }

  void addMod(ofxMarkSynth::ModPtr modPtr) {
    modPtrs.insert({ modPtr->getName(), modPtr });
    if (modPtr) {
      modPtr->doneModLoad();
    }
  }

  DrawingLayerPtr addDrawingLayer(std::string name,
                                 glm::vec2 size,
                                 GLint internalFormat,
                                 int wrap,
                                 bool clearOnUpdate,
                                 ofBlendMode blendMode,
                                 bool useStencil,
                                 int numSamples,
                                 bool isDrawn = true,
                                 bool isOverlay = false,
                                 const std::string& description = "");

  void addConnections(const std::string& dsl);
  void configureGui(std::shared_ptr<ofAppBaseWindow> windowPtr);
  ofParameterGroup& getIntentParameterGroup() { return intentController->getParameterGroup(); }
  void addLiveTexturePtrFn(std::string name, std::function<const ofTexture*()> textureAccessor);
  
  ofParameterGroup& getLayerAlphaParameters() { return layerController->getAlphaParameterGroup(); }
  ofParameterGroup& getLayerPauseParameters() { return layerController->getPauseParameterGroup(); }
  const std::vector<std::shared_ptr<ofParameter<bool>>>& getLayerPauseParamPtrs() const { return layerController->getPauseParamPtrs(); }
  size_t getLayerCount() const { return layerController->getCount(); }
  const DrawingLayerPtrMap& getDrawingLayers() const { return layerController->getLayers(); }
  
  std::optional<std::reference_wrapper<ofAbstractParameter>> findParameterByNamePrefix(const std::string& name) override;

  bool loadFromConfig(const std::string& filepath);
  bool saveModsToCurrentConfig();
  void unload();
  void switchToConfig(const std::string& filepath, bool useCrossfade = true);
  void loadFirstPerformanceConfig();

  void setIntentPresets(const std::vector<IntentPtr>& presets);
  void setIntentStrength(float value);
  void setIntentActivation(size_t index, float value);
  size_t getIntentCount() const { return intentController->getCount(); }

  static void setArtefactRootPath(const std::filesystem::path& root);
  static std::string saveArtefactFilePath(const std::string& relative);
  static void setConfigRootPath(const std::filesystem::path& root);
  static std::string saveConfigFilePath(const std::string& relative);

  float getAgency() const override;
  void setAgency(float agency) { agencyParameter = agency; }
  float getAutoAgencyAggregate() const { return autoAgencyAggregatePrev; }
  float getManualBiasDecaySec() const { return manualBiasDecaySecParameter; }
  float getBaseManualBias() const { return baseManualBiasParameter; }
  float getHibernationFadeDurationSec() const;

  // Agency-triggered register-shift indicator (latched for GUI display).
  float getSecondsSinceAgencyRegisterShift() const {
    if (lastAgencyRegisterShiftTimeSec < 0.0f) return std::numeric_limits<float>::infinity();
    return ofGetElapsedTimef() - lastAgencyRegisterShiftTimeSec;
  }
  int getLastAgencyRegisterShiftCount() const { return lastAgencyRegisterShiftCount; }
  static constexpr int MAX_AGENCY_REGISTER_SHIFT_IDS = 8;
  int getLastAgencyRegisterShiftIdCount() const { return lastAgencyRegisterShiftIdCount; }
  int getLastAgencyRegisterShiftId(int index) const {
    if (index < 0 || index >= lastAgencyRegisterShiftIdCount) return -1;
    return lastAgencyRegisterShiftIds[static_cast<size_t>(index)];
  }

  const std::shared_ptr<ofxAudioAnalysisClient::LocalGistClient>& getAudioAnalysisClient() const { return audioAnalysisClientPtr; }
  PerformanceNavigator& getPerformanceNavigator() { return performanceNavigator; }
  const PerformanceNavigator& getPerformanceNavigator() const { return performanceNavigator; }

  struct PerformerCues {
    bool audioEnabled { false };
    bool videoEnabled { false };
  };
  void setPerformerCues(bool audioEnabled, bool videoEnabled) { performerCues = { audioEnabled, videoEnabled }; }
  PerformerCues getPerformerCues() const { return performerCues; }

  void receive(int sinkId, const glm::vec4& v) override;
  void receive(int sinkId, const float& v) override;
  void update() override;
  glm::vec2 getSize() const { return compositeRenderer->getCompositeSize(); }
  const ofFbo& getCompositeFbo() const { return compositeRenderer->getCompositeFbo(); }
  void draw() override;

  void toggleRecording();
  bool isRecording() const;
  const std::string& getCurrentConfigPath() const { return currentConfigPath; }
  std::string getCurrentConfigId() const;
  void saveImage();
  void requestSaveAllMemories();
  int getActiveSaveCount() const;
  bool keyPressed(int key) override;
  bool keyReleased(int key);

  bool loadModSnapshotSlot(int slotIndex);
  bool toggleLayerPauseSlot(int layerIndex);

  static constexpr int SOURCE_COMPOSITE_FBO = 1;
  static constexpr int SOURCE_MEMORY = 10;

  static constexpr int SINK_BACKGROUND_COLOR = 100;
  static constexpr int SINK_RESET_RANDOMNESS = 200;
  static constexpr int SINK_AGENCY_AUTO = 201;
  
  // Memory bank controller accessor (for Gui)
  MemoryBankController& getMemoryBankController() { return *memoryBankController; }

  ofEvent<HibernationController::CompleteEvent>& getHibernationCompleteEvent();
  HibernationController::State getHibernationState() const;

  class ConfigUnloadEvent : public ofEventArgs {
  public:
    std::string previousConfigPath;
  };

  class ConfigLoadedEvent : public ofEventArgs {
  public:
    std::string newConfigPath;
  };

  ofEvent<ConfigUnloadEvent> configWillUnloadEvent;
  ofEvent<ConfigLoadedEvent> configDidLoadEvent;

  friend class Gui;
  friend class NodeEditorModel;
  friend class NodeEditorLayout;
  friend class SynthConfigSerializer;
  friend class PerformanceNavigator;

  // >>> Time tracking system (public interface)
  // Three time values, all in seconds:
  // 1. Clock Time Since First Run: Wall clock since first cancelHibernation() (never pauses)
  // 2. Synth Running Time: Accumulated time synth has been running (pauses with synth)
  // 3. Config Running Time: Accumulated time current config has been running (resets on config load, pauses with synth)
  
  bool hasEverRun() const;
  float getClockTimeSinceFirstRun() const;
  float getSynthRunningTime() const;
  float getConfigRunningTime() const;
  
  // Convenience accessors for config time (used by Mods and external controllers)
  int getConfigRunningMinutes() const;
  int getConfigRunningSeconds() const;
  // <<<

protected:
  void initParameters() override;

private:
  // Constructor helpers
  void initControllers(const std::string& name, bool startHibernated);
  void initRendering(glm::vec2 compositeSize);
  void initResourcePaths();
  void initPerformanceNavigator();
  void initSinkSourceMappings();
  
  // Serialization helpers
  void updateModConfigJson(nlohmann::ordered_json& modJson, const ModPtr& modPtr);
  static std::filesystem::path artefactRootPath;
  static bool artefactRootPathSet;
  static std::filesystem::path configRootPath;
  static bool configRootPathSet;

  ResourceManager resources;
  std::shared_ptr<ofxAudioAnalysisClient::LocalGistClient> audioAnalysisClientPtr;

  Gui gui;
  PerformanceNavigator performanceNavigator { this };
  PerformerCues performerCues;

  ModPtrMap modPtrs;

  // Cache of per-Mod UI/debug state, preserved across config reloads.
  // Keyed by Mod name (global across configs).
  std::unordered_map<std::string, Mod::UiState> modUiStateCache;
  void captureModUiStateCache();
  void restoreModUiStateCache();

  // Cache of per-Mod ephemeral runtime state, preserved across config reloads.
  // Keyed by Mod name (global across configs).
  std::unordered_map<std::string, Mod::RuntimeState> modRuntimeStateCache;
  void captureModRuntimeStateCache();
  void restoreModRuntimeStateCache();

  // >>> Layer system (delegated to helper class)
  std::unique_ptr<LayerController> layerController;
  // <<<

  // >>> Display and composite rendering (delegated to helper classes)
  std::unique_ptr<DisplayController> displayController;
  std::unique_ptr<CompositeRenderer> compositeRenderer;
  std::unique_ptr<CueGlyphController> cueGlyphController;
  // <<<
  
  std::map<std::string, std::function<const ofTexture*()>> liveTexturePtrFns;

  bool paused;

  void updateDebugViewFbo();
  
  // Debug view system - renders Mod::draw() calls to FBO for ImGui display
  ofFbo debugViewFbo;
  bool debugViewEnabled { false };
  DebugViewMode debugViewMode { DebugViewMode::Fbo };
  static constexpr float DEBUG_VIEW_SIZE { 640.0f };
  
public:
  const ofFbo& getDebugViewFbo() const { return debugViewFbo; }
  bool isDebugViewEnabled() const { return debugViewEnabled; }
  void setDebugViewEnabled(bool enabled) { debugViewEnabled = enabled; }
  void toggleDebugView() { debugViewEnabled = !debugViewEnabled; }
  
private:

  // >>> Config transition crossfade system (delegated to helper class)
  std::unique_ptr<ConfigTransitionManager> configTransitionManager;
  // <<<

  // >>> Intent system (delegated to helper class)
  std::unique_ptr<IntentController> intentController;
  const Intent& getActiveIntent() const { return intentController->getActiveIntent(); }
  float getIntentStrength() const { return intentController->getStrength(); }
  void applyIntent(const Intent& intent, float intentStrength) override;
  void applyIntentToAllMods();
  // <<<

  ofParameter<float> agencyParameter { "Synth Agency", 0.0, 0.0, 1.0 }; // 0.0 -> fully manual; 1.0 -> fully autonomous
  float autoAgencyAggregatePrev { 0.0f };      // Used for current frame getAgency() (intentionally 1-frame delayed)
  float autoAgencyAggregateThisFrame { 0.0f }; // Max of .AgencyAuto inputs received this frame

  // Agency-triggered register shifts (any AgencyController Trigger fired this frame).
  float lastAgencyRegisterShiftTimeSec { -1.0f };
  int lastAgencyRegisterShiftCount { 0 };
  int lastAgencyRegisterShiftIdCount { 0 };
  std::array<int, MAX_AGENCY_REGISTER_SHIFT_IDS> lastAgencyRegisterShiftIds {};

  ofParameter<float> manualBiasDecaySecParameter { "Manual Decay Time", 0.8, 0.1, 5.0 }; // Time for manual control to decay back
  ofParameter<float> baseManualBiasParameter { "Manual Bias Min", 0.1, 0.0, 0.5 }; // Minimum manual control influence
  
  // Background color (part of Intent system, stays in Synth)
  ofParameter<ofFloatColor> backgroundColorParameter { "BackgroundColour", ofFloatColor { 0.0, 0.0, 0.0, 1.0 }, ofFloatColor { 0.0, 0.0, 0.0, 1.0 }, ofFloatColor { 1.0, 1.0, 1.0, 1.0 } };
  ParamController<ofFloatColor> backgroundColorController { backgroundColorParameter };
  ofParameter<float> backgroundBrightnessParameter { "BackgroundBrightness", 0.035f, 0.0f, 1.0f };

  ofxLabel recorderStatus;
  ofxLabel saveStatus;
  ofxLabel pauseStatus;

  std::string currentConfigPath;
  std::optional<std::string> pendingStartupConfigPath;

  bool guiVisible { true };
  bool initialLoadCallbackEmitted { false };

  std::shared_ptr<LoggerChannel> loggerChannelPtr;

  // >>> Hibernation and time tracking (delegated to helper classes)
  std::unique_ptr<HibernationController> hibernationController;
  std::unique_ptr<TimeTracker> timeTracker;
  // <<<

#ifdef TARGET_MAC
  std::unique_ptr<VideoRecorder> videoRecorderPtr;
#endif

  std::unique_ptr<AsyncImageSaver> imageSaver;
  
  // Deferred image save: flag set in keyPressed, processed after composite update
  bool pendingImageSave { false };
  std::string pendingImageSavePath;

  // >>> Memory bank system (delegated to helper class)
  std::unique_ptr<MemoryBankController> memoryBankController;
  // <<<
};

template <typename ModT>
ofxMarkSynth::ModPtr Synth::addMod(const std::string& name, ofxMarkSynth::ModConfig&& modConfig) {
  auto self = std::static_pointer_cast<Synth>(shared_from_this());
  auto modPtr = std::make_shared<ModT>(self, name, std::forward<ofxMarkSynth::ModConfig>(modConfig));
  addMod(modPtr);
  return modPtr;
}

template <typename ModT, typename... Args>
ofxMarkSynth::ModPtr Synth::addMod(const std::string& name, ofxMarkSynth::ModConfig&& modConfig, Args&&... args) {
  auto self = std::static_pointer_cast<Synth>(shared_from_this());
  auto modPtr = std::make_shared<ModT>(self, name, std::forward<ofxMarkSynth::ModConfig>(modConfig), std::forward<Args>(args)...);
  addMod(modPtr);
  return modPtr;
}

} // ofxMarkSynth
