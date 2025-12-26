//
//  Synth.hpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#pragma once

#include "Mod.hpp"
#include "TonemapCrossfadeShader.h"
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "util/AsyncImageSaver.hpp"
#include "Intent.hpp"
#include "ParamController.h"
#include "Gui.hpp"
#include "nodeEditor/NodeEditorModel.hpp"
#include "nodeEditor/NodeEditorLayout.hpp"
#include "util/LoggerChannel.hpp"
#include "util/ModFactory.hpp"
#include "util/PerformanceNavigator.hpp"
#include "util/MemoryBank.hpp"
#include "util/VideoRecorder.hpp"
#include "util/HibernationController.hpp"
#include "util/TimeTracker.hpp"
#include "util/ConfigTransitionManager.hpp"
#include "util/IntentController.hpp"

namespace ofxAudioAnalysisClient {
class LocalGistClient;
}

namespace ofxMarkSynth {

const ofFloatColor DEFAULT_CLEAR_COLOR { 0.0, 0.0, 0.0, 0.0 };

using DrawingLayerPtrMap = std::map<std::string, DrawingLayerPtr>;
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
  
  ofParameterGroup& getLayerAlphaParameters() { return layerAlphaParameters; };
  
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

  float getAgency() const override { return agencyParameter; }
  void setAgency(float agency) { agencyParameter = agency; }
  float getManualBiasDecaySec() const { return manualBiasDecaySecParameter; }
  float getBaseManualBias() const { return baseManualBiasParameter; }
  float getHibernationFadeDurationSec() const;

  const std::shared_ptr<ofxAudioAnalysisClient::LocalGistClient>& getAudioAnalysisClient() const { return audioAnalysisClientPtr; }
  PerformanceNavigator& getPerformanceNavigator() { return performanceNavigator; }
  const PerformanceNavigator& getPerformanceNavigator() const { return performanceNavigator; }

  void receive(int sinkId, const glm::vec4& v) override;
  void receive(int sinkId, const float& v) override;
  void update() override;
  glm::vec2 getSize() const { return compositeSize; }
  const ofFbo& getCompositeFbo() const { return imageCompositeFbo; }
  void draw() override;

  void toggleRecording();
  bool isRecording() const;
  const std::string& getCurrentConfigPath() const { return currentConfigPath; }
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

  // Memory bank sinks
  static constexpr int SINK_MEMORY_SAVE = 300;
  static constexpr int SINK_MEMORY_SAVE_SLOT = 301;
  static constexpr int SINK_MEMORY_EMIT = 302;
  static constexpr int SINK_MEMORY_EMIT_SLOT = 303;
  static constexpr int SINK_MEMORY_EMIT_RANDOM = 304;
  static constexpr int SINK_MEMORY_EMIT_RANDOM_NEW = 305;
  static constexpr int SINK_MEMORY_EMIT_RANDOM_OLD = 306;
  static constexpr int SINK_MEMORY_SAVE_CENTRE = 307;
  static constexpr int SINK_MEMORY_SAVE_WIDTH = 308;
  static constexpr int SINK_MEMORY_EMIT_CENTRE = 309;
  static constexpr int SINK_MEMORY_EMIT_WIDTH = 310;
  static constexpr int SINK_MEMORY_CLEAR_ALL = 311;

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
  static std::filesystem::path artefactRootPath;
  static bool artefactRootPathSet;
  static std::filesystem::path configRootPath;
  static bool configRootPathSet;

  ResourceManager resources;
  std::shared_ptr<ofxAudioAnalysisClient::LocalGistClient> audioAnalysisClientPtr;

  Gui gui;
  PerformanceNavigator performanceNavigator { this };

  ModPtrMap modPtrs;
  DrawingLayerPtrMap drawingLayerPtrs;
  std::unordered_map<std::string, float> initialLayerAlphas;
  std::unordered_map<std::string, bool> initialLayerPaused;

  void initDisplayParameterGroup();
  void initFboParameterGroup();
  void initLayerPauseParameterGroup();
  
  std::map<std::string, std::function<const ofTexture*()>> liveTexturePtrFns;

  bool paused;

  TonemapCrossfadeShader tonemapCrossfadeShader;
  ofMesh crossfadeQuadMesh;
  ofMesh unitQuadMesh;
  glm::vec2 compositeSize;
  float compositeScale;
  ofFbo imageCompositeFbo;
  void updateCompositeImage();

  float sidePanelWidth, sidePanelHeight;
  PingPongFbo leftPanelFbo, rightPanelFbo;

  float leftSidePanelLastUpdate { 0.0 };
  float rightSidePanelLastUpdate { 0.0 };
  float leftSidePanelTimeoutSecs { 7.0 };
  float rightSidePanelTimeoutSecs { 11.0 };
  void updateSidePanels();

  void drawMiddlePanel(float w, float h, float scale);
  void drawSidePanels(float xleft, float xright, float w, float h);
  void updateDebugViewFbo();
  
  // Debug view system - renders Mod::draw() calls to FBO for ImGui display
  ofFbo debugViewFbo;
  bool debugViewEnabled { false };
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
  ofParameter<float> manualBiasDecaySecParameter { "Manual Decay Time", 0.8, 0.1, 5.0 }; // Time for manual control to decay back
  ofParameter<float> baseManualBiasParameter { "Manual Bias Min", 0.1, 0.0, 0.5 }; // Minimum manual control influence
  
  ofParameterGroup layerAlphaParameters;
  std::vector<std::shared_ptr<ofParameter<float>>> layerAlphaParamPtrs;
  ofParameterGroup layerPauseParameters;
  std::vector<std::shared_ptr<ofParameter<bool>>> layerPauseParamPtrs;
  ofParameterGroup displayParameters;
  ofParameter<ofFloatColor> backgroundColorParameter { "BackgroundColour", ofFloatColor { 0.0, 0.0, 0.0, 1.0 }, ofFloatColor { 0.0, 0.0, 0.0, 1.0 }, ofFloatColor { 1.0, 1.0, 1.0, 1.0 } };
  ParamController<ofFloatColor> backgroundColorController { backgroundColorParameter };
  ofParameter<float> backgroundMultiplierParameter { "BackgroundMultiplier", 0.1, 0.0, 1.0 };
  ofParameter<int> toneMapTypeParameter { "Tone map", 3, 0, 5 }; // 0: Linear (clamp); 1: Reinhard; 2: Reinhard Extended; 3: ACES; 4: Filmic; 5: Exposure
  ofParameter<float> exposureParameter { "Exposure", 1.0, 0.0, 4.0 };

  ofParameter<float> gammaParameter { "Gamma", 2.2, 0.1, 5.0 };
  ofParameter<float> whitePointParameter { "White Pt", 11.2, 1.0, 20.0 }; // for Reinhard Extended
  ofParameter<float> contrastParameter { "Contrast", 1.03, 0.9, 1.1 };
  ofParameter<float> saturationParameter { "Saturation", 1.0, 0.0, 2.0 };
  ofParameter<float> brightnessParameter { "Brightness", 0.0, -0.1, 0.1 };
  ofParameter<float> hueShiftParameter { "Hue Shift", 0.0, -1.0, 1.0 };
  ofParameter<float> sideExposureParameter { "Side Exp", 0.6, 0.0, 4.0 };
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
  void updateLayerPauseStates();
  // <<<

#ifdef TARGET_MAC
  std::unique_ptr<VideoRecorder> videoRecorderPtr;
#endif

  std::unique_ptr<AsyncImageSaver> imageSaver;
  
  // Deferred image save: flag set in keyPressed, processed after composite update
  bool pendingImageSave { false };
  std::string pendingImageSavePath;

  // >>> Memory bank system
  bool globalMemoryBankLoaded { false };
  bool memorySaveAllRequested { false };
  MemoryBank memoryBank;
  void initMemoryBankParameterGroup();

  // Memory save parameters
  ofParameterGroup memoryBankParameters;
  ofParameter<float> memorySaveCentreParameter { "MemorySaveCentre", 1.0, 0.0, 1.0 };
  ofParameter<float> memorySaveWidthParameter { "MemorySaveWidth", 0.0, 0.0, 1.0 };
  ParamController<float> memorySaveCentreController { memorySaveCentreParameter };
  ParamController<float> memorySaveWidthController { memorySaveWidthParameter };

  // Memory emit parameters
  ofParameter<float> memoryEmitCentreParameter { "MemoryEmitCentre", 0.5, 0.0, 1.0 };
  ofParameter<float> memoryEmitWidthParameter { "MemoryEmitWidth", 1.0, 0.0, 1.0 };
  ParamController<float> memoryEmitCentreController { memoryEmitCentreParameter };
  ParamController<float> memoryEmitWidthController { memoryEmitWidthParameter };

  // Emit rate limiting
  float lastMemoryEmitTime { 0.0f };
  ofParameter<float> memoryEmitMinIntervalParameter { "MemoryEmitMinInterval", 0.1, 0.0, 2.0 };
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
