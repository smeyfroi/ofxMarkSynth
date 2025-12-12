//
//  Synth.hpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#pragma once

#include "Mod.hpp"
#ifdef TARGET_MAC
#include "ofxFFmpegRecorder.h"
#endif
#include "TonemapCrossfadeShader.h"
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "SaveToFileThread.hpp"
#include "Intent.hpp"
#include "ParamController.h"
#include "Gui.hpp"
#include "NodeEditorModel.hpp"
#include "NodeEditorLayout.hpp"
#include "LoggerChannel.hpp"
#include "util/ModFactory.hpp"
#include "util/PerformanceNavigator.hpp"
#include "util/MemoryBank.hpp"

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
  static std::shared_ptr<Synth> create(const std::string& name, ModConfig config, bool startPaused, glm::vec2 compositeSize, ResourceManager resources = {});

protected:
  // The composite is the middle (square) section, scaled to fit the window height
  Synth(const std::string& name,
        ModConfig config,
        bool startPaused,
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
  ofParameterGroup& getIntentParameterGroup() { return intentParameters; }
  void addLiveTexturePtrFn(std::string name, std::function<const ofTexture*()> textureAccessor);

  std::optional<std::reference_wrapper<ofAbstractParameter>> findParameterByNamePrefix(const std::string& name) override;

  bool loadFromConfig(const std::string& filepath);
  bool saveModsToCurrentConfig();
  void unload();
  void switchToConfig(const std::string& filepath, bool useCrossfade = true);
  void loadFirstPerformanceConfig();
  void setIntentPresets(const std::vector<IntentPtr>& presets);

  static void setArtefactRootPath(const std::filesystem::path& root);
  static std::string saveArtefactFilePath(const std::string& relative);
  static void setConfigRootPath(const std::filesystem::path& root);
  static std::string saveConfigFilePath(const std::string& relative);

  float getAgency() const override { return agencyParameter; }
  void setAgency(float agency) { agencyParameter = agency; }
  float getManualBiasDecaySec() const { return manualBiasDecaySecParameter; }
  float getBaseManualBias() const { return baseManualBiasParameter; }
  float getHibernationFadeDurationSec() const { return hibernationFadeDurationParameter; }

  const std::shared_ptr<ofxAudioAnalysisClient::LocalGistClient>& getAudioAnalysisClient() const { return audioAnalysisClientPtr; }

  void receive(int sinkId, const glm::vec4& v) override;
  void receive(int sinkId, const float& v) override;
  void update() override;
  glm::vec2 getSize() const { return compositeSize; }
  const ofFbo& getCompositeFbo() const { return imageCompositeFbo; }
  void draw() override;

  void toggleRecording();
  void saveImage();
  bool keyPressed(int key) override;
  bool keyReleased(int key);

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

  class HibernationCompleteEvent : public ofEventArgs {
  public:
    float fadeDuration;
    std::string synthName;
  };
  ofEvent<HibernationCompleteEvent> hibernationCompleteEvent;

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
  void initIntentParameterGroup();
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
  void drawDebugViews();

  // >>> Config transition crossfade system
  enum class TransitionState {
    NONE,           // No transition in progress
    CROSSFADING     // Crossfading from snapshot to live
  };
  TransitionState transitionState { TransitionState::NONE };
  ofFbo transitionSnapshotFbo;  // Holds captured frame from old config
  float transitionStartTime { 0.0f };
  float transitionAlpha { 0.0f };  // 0.0 = all snapshot, 1.0 = all live
  ofParameter<float> crossfadeDurationParameter { "Crossfade Duration", 2.5, 0.5, 10.0 };
  void updateTransition();
  void captureSnapshot();
  // <<<

  // Intent system
  IntentActivations intentActivations;
  Intent activeIntent { "Active", 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
  ofParameterGroup intentParameters;
  ofParameter<float> intentStrengthParameter { "Intent Strength", 0.0, 0.0, 1.0 };
  std::vector<std::shared_ptr<ofParameter<float>>> intentActivationParameters;
  ofxLabel activeIntentInfoLabel1, activeIntentInfoLabel2;
  const Intent& getActiveIntent() const { return activeIntent; }
  float getIntentStrength() const { return intentStrengthParameter; }
  void applyIntent(const Intent& intent, float intentStrength) override;
  void updateIntentActivations();
  void computeActiveIntent();
  void applyIntentToAllMods();

  ofParameter<float> agencyParameter { "Synth Agency", 0.0, 0.0, 1.0 }; // 0.0 -> fully manual; 1.0 -> fully autonomous
  ofParameter<float> manualBiasDecaySecParameter { "Manual Decay Time", 0.8, 0.1, 5.0 }; // Time for manual control to decay back
  ofParameter<float> baseManualBiasParameter { "Manual Bias Min", 0.1, 0.0, 0.5 }; // Minimum manual control influence
  ofParameterGroup fboParameters;
  std::vector<std::shared_ptr<ofParameter<float>>> fboParamPtrs;
  ofParameterGroup layerPauseParameters;
  std::vector<std::shared_ptr<ofParameter<bool>>> layerPauseParamPtrs;
  ofParameterGroup displayParameters;
  ofParameter<ofFloatColor> backgroundColorParameter { "BackgroundColour", ofFloatColor { 0.0, 0.0, 0.0, 1.0 }, ofFloatColor { 0.0, 0.0, 0.0, 1.0 }, ofFloatColor { 1.0, 1.0, 1.0, 1.0 } };
  ParamController<ofFloatColor> backgroundColorController { backgroundColorParameter };
  ofParameter<float> backgroundMultiplierParameter { "BackgroundMultiplier", 0.1, 0.0, 1.0 };
  ofParameter<int> toneMapTypeParameter { "Tone map", 3, 0, 5 }; // 0: Linear (clamp); 1: Reinhard; 2: Reinhard Extended; 3: ACES; 4: Filmic; 5: Exposure
  ofParameter<float> exposureParameter { "Exposure", 1.0, 0.0, 4.0 };
  ofParameter<float> layerPauseFadeDurationSecParameter { "Layer Pause Fade", 0.5, 0.0, 10.0 };
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

  // >>> Hibernation system
  enum class HibernationState {
    ACTIVE,        // Normal operation
    FADING_OUT,    // Hibernating - fade in progress
    HIBERNATED     // Fully hibernated (black screen, paused)
  };
  HibernationState hibernationState { HibernationState::ACTIVE };
  float hibernationAlpha { 1.0f };  // 1.0 = fully visible, 0.0 = fully black
  float hibernationStartTime { 0.0f };
  ofParameter<float> hibernationFadeDurationParameter { "Hibernate Duration", 2.0, 0.5, 10.0 };
  void startHibernation();
  void cancelHibernation();
  void updateHibernation();
  void updateLayerPauseStates();

  bool isHibernating() const { return hibernationState != HibernationState::ACTIVE; }
  std::string getHibernationStateString() const;
  HibernationState getHibernationState() const { return hibernationState; }
  float getHibernationStartTime() const { return hibernationStartTime; }
  // <<<

#ifdef TARGET_MAC
  ofxFFmpegRecorder recorder;
  glm::vec2 recorderCompositeSize;
  ofFbo recorderCompositeFbo;

  // PBO-based async pixel readback for video recording
  static constexpr int NUM_PBOS { 2 };
  ofBufferObject recorderPbos[NUM_PBOS];
  int recorderPboWriteIndex { 0 };  // PBO currently being written to by GPU
  int recorderFrameCount { 0 };      // Frames captured since recording started
  ofPixels recorderPixels;           // Reusable pixel buffer
#endif

  std::vector<std::unique_ptr<SaveToFileThread>> saveToFileThreads;
  void pruneSaveThreads();

  // PBO-based async pixel readback for image saving
  ofBufferObject imageSavePbo;  // Pre-allocated at construction, reused for all saves
  bool imageSaveRequested { false };  // Set by saveImage(), cleared after glReadPixels issued
  struct PendingImageSave {
    GLsync fence { nullptr };
    std::string timestamp;  // Captured at save request, filepath generated later
    int framesSinceRequested { 0 };
  };
  std::optional<PendingImageSave> pendingImageSave;
  void initiateImageSaveTransfer();  // Called from draw() after all rendering
  void processPendingImageSave();
  float saveStatusClearTime { 0.0f };

  // >>> Memory bank system
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
