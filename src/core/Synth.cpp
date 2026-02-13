//
//  Synth.cpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#include "core/Synth.hpp"
#include "core/SynthConstants.h"
#include "processMods/AgencyControllerMod.hpp"
#include "ofxTimeMeasurements.h"
#include "ofConstants.h"
#include "ofUtils.h"
#include "core/Gui.hpp"
#include "config/SynthConfigSerializer.hpp"
#include "config/Parameter.hpp"
#include "sourceMods/AudioDataSourceMod.hpp"
#include "sourceMods/VideoFlowSourceMod.hpp"
#include "util/TimeStringUtil.h"
#include "ofxImGui.h"
#include "ofxAudioAnalysisClient.h"
#include "nlohmann/json.hpp"
#include <algorithm>
#include <fstream>



// undefine to send logs to the imgui
#define OF_LOGGING_ENABLED



namespace ofxMarkSynth {



std::filesystem::path Synth::artefactRootPath{};
bool Synth::artefactRootPathSet = false;

void Synth::setArtefactRootPath(const std::filesystem::path& root) {
  artefactRootPath = root;
  artefactRootPathSet = true;
}

std::string Synth::saveArtefactFilePath(const std::string& filename) {
  if (!artefactRootPathSet) {
    ofLogError("Synth") << "performanceArtefactRootPath not set in ResourceManager";
    return filename;
  }
  auto path = artefactRootPath/filename;
  auto directory = path.parent_path();
  std::filesystem::create_directories(directory);
  return path.string();
}

std::filesystem::path Synth::configRootPath{};
bool Synth::configRootPathSet = false;

void Synth::setConfigRootPath(const std::filesystem::path& root) {
  configRootPath = root;
  configRootPathSet = true;
}

std::string Synth::saveConfigFilePath(const std::string& filename) {
  if (!configRootPathSet) {
    ofLogError("Synth") << "performanceConfigRootPath not set in ResourceManager";
    return filename;
  }
  auto path = configRootPath/filename;
  auto directory = path.parent_path();
  std::filesystem::create_directories(directory);
  return path.string();
}

constexpr std::string SNAPSHOTS_FOLDER_NAME = "drawing";
constexpr std::string AUTO_SNAPSHOTS_FOLDER_NAME = "drawing-auto";
constexpr std::string VIDEOS_FOLDER_NAME = "drawing-recording";
// Also: camera-recording, mic-recording
// Also: ModSnapshotManager uses "mod-params/snapshots" and NodeEditorLayoutManager uses "node-layouts"


static std::shared_ptr<ofxAudioAnalysisClient::LocalGistClient> createAudioAnalysisClient(const ResourceManager& resources) {
  auto sourceAudioPathPtr = resources.get<std::filesystem::path>("sourceAudioPath");
  if (sourceAudioPathPtr && !sourceAudioPathPtr->empty()) {
    auto outDeviceNamePtr = resources.get<std::string>("audioOutDeviceName");
    auto bufferSizePtr = resources.get<int>("audioBufferSize");
    auto nChannelsPtr = resources.get<int>("audioChannels");
    auto sampleRatePtr = resources.get<int>("audioSampleRate");
    auto sourceAudioStartPositionPtr = resources.get<std::string>("sourceAudioStartPosition");

    if (!outDeviceNamePtr || !bufferSizePtr || !nChannelsPtr || !sampleRatePtr) {
      ofLogError("Synth")
          << "Missing required audio resources for file playback: sourceAudioPath requires audioOutDeviceName, audioBufferSize, audioChannels, audioSampleRate";
      return nullptr;
    }

    auto client = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>(
        *sourceAudioPathPtr, *outDeviceNamePtr, *bufferSizePtr, *nChannelsPtr, *sampleRatePtr);
    
    if (sourceAudioStartPositionPtr && !sourceAudioStartPositionPtr->empty()) {
      int seconds = parseTimeStringToSeconds(*sourceAudioStartPositionPtr);
      if (seconds > 0) {
        client->setPositionSeconds(seconds);
      }
    }
    
    return client;
  }

  auto micDeviceNamePtr = resources.get<std::string>("micDeviceName");
  auto recordAudioPtr = resources.get<bool>("recordAudio");
  auto recordingPathPtr = resources.get<std::filesystem::path>("audioRecordingPath");
  if (micDeviceNamePtr && recordAudioPtr && recordingPathPtr) {
    std::error_code ec;
    std::filesystem::create_directories(*recordingPathPtr, ec);
    if (ec) {
      ofLogWarning("Synth") << "Failed to create audioRecordingPath: " << *recordingPathPtr << " (" << ec.message() << ")";
    }

    return std::make_shared<ofxAudioAnalysisClient::LocalGistClient>(*micDeviceNamePtr, *recordAudioPtr, *recordingPathPtr);
  }

  ofLogError("Synth")
      << "Missing required audio source resources: provide either (sourceAudioPath + audioOutDeviceName + audioBufferSize + audioChannels + audioSampleRate) or (micDeviceName + recordAudio + audioRecordingPath)";
  return nullptr;
}



std::shared_ptr<Synth> Synth::create(const std::string& name, ModConfig config, bool startHibernated, glm::vec2 compositeSize, ResourceManager resources) {
  auto audioClient = createAudioAnalysisClient(resources);
  if (!audioClient) {
    ofLogError("Synth") << "Synth::create: failed to create audio source";
    return nullptr;
  }

  auto synth = std::shared_ptr<Synth>(
      new Synth(name,
                std::move(config),
                startHibernated,
                compositeSize,
                std::move(audioClient),
                std::move(resources)));

  // Safe place to load startup config: synth is now owned by a shared_ptr.
  // Use switchToConfig (not loadFromConfig) so Mods are fully initialized before first update.
  if (synth && synth->pendingStartupConfigPath && !synth->pendingStartupConfigPath->empty()) {
    synth->switchToConfig(*synth->pendingStartupConfigPath, false);
    synth->pendingStartupConfigPath.reset();
  }

  return synth;
}



Synth::Synth(const std::string& name_, ModConfig config, bool startHibernated, glm::vec2 compositeSize_, std::shared_ptr<ofxAudioAnalysisClient::LocalGistClient> audioAnalysisClient, ResourceManager resources_) :
Mod(nullptr, name_, std::move(config)),
paused { startHibernated },  // Start paused if hibernated
resources { std::move(resources_) },
audioAnalysisClientPtr { std::move(audioAnalysisClient) }
{
  initControllers(startHibernated);
  initRendering(compositeSize_);
  initResourcePaths();
  initPerformanceNavigator();
  initSinkSourceMappings();

  // Enable node editor tooltips / contribution weights for background colour.
  registerControllerForSource(backgroundColorParameter, backgroundColorController);
}

void Synth::initControllers(bool startHibernated) {
  hibernationController = std::make_unique<HibernationController>(startHibernated);
  timeTracker = std::make_unique<TimeTracker>();
  configTransitionManager = std::make_unique<ConfigTransitionManager>();
  intentController = std::make_unique<IntentController>();
  layerController = std::make_unique<LayerController>();
  cueGlyphController = std::make_unique<CueGlyphController>();
}

void Synth::initRendering(glm::vec2 compositeSize) {
  displayController = std::make_unique<DisplayController>();
  displayController->buildParameterGroup();
  
  compositeRenderer = std::make_unique<CompositeRenderer>();
  compositeRenderer->allocate(compositeSize, ofGetWindowWidth(), ofGetWindowHeight(),
                              *resources.get<float>("compositePanelGapPx"));
  
  imageSaver = std::make_unique<AsyncImageSaver>(compositeSize);
  
#ifdef TARGET_MAC
  videoRecorderPtr = std::make_unique<VideoRecorder>();
  videoRecorderPtr->setup(
      *resources.get<glm::vec2>("recorderCompositeSize"),
      *resources.get<std::filesystem::path>("ffmpegBinaryPath"));
#endif
}

void Synth::initResourcePaths() {
  if (resources.has("performanceArtefactRootPath")) {
    auto p = resources.get<std::filesystem::path>("performanceArtefactRootPath");
    if (p) {
      if (!std::filesystem::exists(*p)) {
        ofLogError("Synth") << "performanceArtefactRootPath does not exist: " << *p;
      }
      Synth::setArtefactRootPath(*p);
    } else {
      ofLogError("Synth") << "Resource 'performanceArtefactRootPath' present but wrong type";
    }
  } else {
    ofLogError("Synth") << "Missing required resource 'performanceArtefactRootPath'";
  }
  
  if (resources.has("performanceConfigRootPath")) {
    auto p = resources.get<std::filesystem::path>("performanceConfigRootPath");
    if (p) {
      if (!std::filesystem::exists(*p)) {
        ofLogError("Synth") << "performanceConfigRootPath does not exist: " << *p;
      }
      Synth::setConfigRootPath(*p);
    } else {
      ofLogError("Synth") << "Resource 'performanceConfigRootPath' present but wrong type";
    }
  } else {
    ofLogError("Synth") << "Missing required resource 'performanceConfigRootPath'";
  }
}

void Synth::initPerformanceNavigator() {
  of::random::seed(0);
  
  // Initialize performance navigator from ResourceManager if path is provided
  if (resources.has("performanceConfigRootPath")) {
    auto pathPtr = resources.get<std::filesystem::path>("performanceConfigRootPath");
    if (pathPtr) {
      performanceNavigator.loadFromFolder(*pathPtr / "synth");
    }
  }

  // Optional: pick a startup config by name (stem, not path) from the performance folder list.
  // NOTE: do not call loadFromConfig() here; shared_from_this() is not valid in the constructor.
  if (resources.has("startupPerformanceConfigName")) {
    auto startupNamePtr = resources.get<std::string>("startupPerformanceConfigName");
    if (startupNamePtr && !startupNamePtr->empty()) {
      if (performanceNavigator.selectConfigByName(*startupNamePtr)) {
        const std::string configPath = performanceNavigator.getCurrentConfigPath();
        if (!configPath.empty()) {
          pendingStartupConfigPath = configPath;
        }
      } else {
        ofLogError("Synth") << "startupPerformanceConfigName not found in performance list: " << *startupNamePtr;
      }
    }
  }
}

void Synth::initSinkSourceMappings() {
  sourceNameIdMap = {
    { "CompositeFbo", SOURCE_COMPOSITE_FBO },
    { "Memory", SOURCE_MEMORY }
  };
  
  memoryBankController = std::make_unique<MemoryBankController>();
  memoryBankController->allocate({ 1024, 1024 });
  
  sinkNameIdMap = {
    { backgroundColorParameter.getName(), SINK_BACKGROUND_COLOR },
    { "ResetRandomness", SINK_RESET_RANDOMNESS },
    { "AgencyAuto", SINK_AGENCY_AUTO }
  };
  
  for (const auto& [name, id] : memoryBankController->getSinkNameIdMap()) {
    sinkNameIdMap[name] = id;
  }
}

// TODO: fold this into loadFromConfig and the ctor?
void Synth::configureGui(std::shared_ptr<ofAppBaseWindow> windowPtr) {
  layerController->buildAlphaParameters();
  layerController->buildPauseParameters();
  memoryBankController->buildParameterGroup();
  
  parameters = getParameterGroup();

  // Pass a windowPtr for a full imgui, else handle it in the calling ofApp
  // FIXME: this also means that child params don't get added into the Synth param group
  if (windowPtr) {
    loggerChannelPtr = std::make_shared<LoggerChannel>();
#ifndef OF_LOGGING_ENABLED
    ofSetLoggerChannel(loggerChannelPtr);
#endif
    gui.setup(dynamic_pointer_cast<Synth>(shared_from_this()), windowPtr);
  } else {
    // Assume that we want Mod params added as child params to the Synth parameter group
    std::for_each(modPtrs.cbegin(), modPtrs.cend(), [this](const auto& pair) {
      const auto& [name, modPtr] = pair;
      ofParameterGroup& pg = modPtr->getParameterGroup();
      if (pg.size() != 0) parameters.add(pg);
    });
  }
  if (!initialLoadCallbackEmitted && !currentConfigPath.empty()) {
    ConfigLoadedEvent ev;
    ev.newConfigPath = currentConfigPath;
    ofNotifyEvent(configDidLoadEvent, ev, this);
    initialLoadCallbackEmitted = true;
  }
}

void Synth::shutdown() {
  ofLogNotice("Synth") << "Synth::shutdown " << name;
  
  std::for_each(modPtrs.cbegin(), modPtrs.cend(), [](const auto& pair) {
    const auto& [name, modPtr] = pair;
    modPtr->shutdown();
  });
  
  gui.exit();
  
#ifdef TARGET_MAC
  if (videoRecorderPtr) {
    videoRecorderPtr->shutdown();
  }
#endif
  
  if (audioAnalysisClientPtr) {
    audioAnalysisClientPtr->stopSegmentRecording();
    audioAnalysisClientPtr->stopRecording();
    audioAnalysisClientPtr->closeStream();
  }

  if (imageSaver) {
    imageSaver->flush();
  }
}

void Synth::unload() {
  ofLogNotice("Synth") << "Synth::unload " << name;

  // 1) Shutdown and clear Mods
  for (auto& kv : modPtrs) {
    const auto& modPtr = kv.second;
    if (modPtr) modPtr->shutdown();
  }
  modPtrs.clear();

  // 2) Clear drawing layers
  layerController->clear();

  // 3) Clear GUI live texture hooks
  liveTexturePtrFns.clear();

  // 4) Clear intent controller state
  intentController->setPresets({});

  // 5) Clear current config path
  currentConfigPath.clear();
  if (hibernationController) {
    hibernationController->setConfigId({});
  }

  // 6) Clear performer cues
  performerCues = {};

  // Note: Keep displayController, compositeRenderer, and other helper classes
  // Rebuild of parameter groups happens when reloading config (configureGui/init* called then)
}

void Synth::captureModUiStateCache() {
  for (const auto& [modName, modPtr] : modPtrs) {
    if (!modPtr) continue;
    modUiStateCache[modName] = modPtr->captureUiState();
  }
}

void Synth::restoreModUiStateCache() {
  for (const auto& [modName, modPtr] : modPtrs) {
    if (!modPtr) continue;

    auto it = modUiStateCache.find(modName);
    if (it == modUiStateCache.end()) continue;

    modPtr->restoreUiState(it->second);
  }
}

void Synth::captureModRuntimeStateCache() {
  for (const auto& [modName, modPtr] : modPtrs) {
    if (!modPtr) continue;
    modRuntimeStateCache[modName] = modPtr->captureRuntimeState();
  }
}

void Synth::restoreModRuntimeStateCache() {
  for (const auto& [modName, modPtr] : modPtrs) {
    if (!modPtr) continue;

    auto it = modRuntimeStateCache.find(modName);
    if (it == modRuntimeStateCache.end()) continue;

    modPtr->restoreRuntimeState(it->second);
  }
}

DrawingLayerPtr Synth::addDrawingLayer(std::string name, glm::vec2 size, GLint internalFormat, int wrap, bool clearOnUpdate, ofBlendMode blendMode, bool useStencil, int numSamples, bool isDrawn, bool isOverlay, const std::string& description) {
  return layerController->addLayer(name, size, internalFormat, wrap, clearOnUpdate, blendMode, useStencil, numSamples, isDrawn, isOverlay, description);
}

void Synth::addConnections(const std::string& dsl) {
  std::istringstream stream(dsl);
  std::string line;
  
  while (std::getline(stream, line)) {
    line = ofTrim(line);
    if (line.empty() || line[0] == '#') continue;
    
    // Parse "sourceMod.sourcePort -> sinkMod.sinkPort"
    auto arrowPos = line.find("->");
    if (arrowPos == std::string::npos) continue;
    
    std::string sourceStr = ofTrim(line.substr(0, arrowPos));
    std::string sinkStr = ofTrim(line.substr(arrowPos + 2));
    
    auto sourceDot = sourceStr.find('.');
    auto sinkDot = sinkStr.find('.');
    
    if (sourceDot == std::string::npos || sinkDot == std::string::npos) continue;
    
    std::string sourceModName = sourceStr.substr(0, sourceDot);
    std::string sourcePortName = sourceStr.substr(sourceDot + 1);
    std::string sinkModName = sinkStr.substr(0, sinkDot);
    std::string sinkPortName = sinkStr.substr(sinkDot + 1);

    // Validator rule: PreScaleExp parameters are config-time only.
    // These sinks are intended for venue/preset tuning and should not be modulated by runtime connections.
    if (sinkPortName.ends_with("PreScaleExp")) {
      ofLogError("Synth") << "Synth::addConnections: Disallowed connection to config-time sink '" << sinkPortName
                          << "' (use the Mod's config/preset instead)";
      continue;
    }
    
    // Look up mods
    if (!sourceModName.empty() && !modPtrs.contains(sourceModName)) {
      ofLogError("Synth") << "Synth::addConnections: Unknown source mod name: " << sourceModName;
      continue;
    }
    auto sourceModPtr = sourceModName.empty() ? shared_from_this() : modPtrs.at(sourceModName);
    
    if(!sinkModName.empty() && !modPtrs.contains(sinkModName)) {
      ofLogError("Synth") << "Synth::addConnections: Unknown sink mod name: " << sinkModName;
      continue;
    }
    auto sinkModPtr = sinkModName.empty() ? shared_from_this() : modPtrs.at(sinkModName);
    
    // Convert port names to IDs
    int sourcePort = sourceModPtr->getSourceId(sourcePortName);
    int sinkPort = sinkModPtr->getSinkId(sinkPortName);
    
    // Create connection
    connectSourceToSinks(sourceModPtr, {
      { sourcePort, {{ sinkModPtr, sinkPort }} }
    });
  }
}

void Synth::addLiveTexturePtrFn(std::string name, std::function<const ofTexture*()> textureAccessor) {
  liveTexturePtrFns[name] = textureAccessor;
}

float Synth::getAgency() const {
  return std::clamp(agencyParameter + autoAgencyAggregatePrev, 0.0f, 1.0f);
}

void Synth::receive(int sinkId, const glm::vec4& v) {
  switch (sinkId) {
    case SINK_BACKGROUND_COLOR:
      backgroundColorController.updateAuto(ofFloatColor { v.r, v.g, v.b, v.a }, getAgency());
      break;
    default:
      ofLogError("Synth") << "glm::vec4 receive for unknown sinkId " << sinkId;
  }
}

void Synth::receive(int sinkId, const float& v) {
  switch (sinkId) {
    case SINK_AGENCY_AUTO:
      autoAgencyAggregateThisFrame = std::max(autoAgencyAggregateThisFrame, std::clamp(v, 0.0f, 1.0f));
      break;

    case SINK_RESET_RANDOMNESS:
      {
        // Use bucketed onset value as seed for repeatability
        int seed = static_cast<int>(v * 10.0f); // Adjust multiplier for desired granularity
        of::random::seed(seed);
      }
      break;
      
    default:
      // Try memory bank controller
      {
        auto result = memoryBankController->handleSink(sinkId, v, compositeRenderer->getCompositeFbo(), getAgency());
        if (result.shouldEmit && result.texture) {
          emit(SOURCE_MEMORY, *result.texture);
          return;
        }
        // If handleSink returned a non-emit result (save/param update), it was handled
        // Check if it was a known memory sink by checking all sink IDs
        auto sinkMap = memoryBankController->getSinkNameIdMap();
        for (const auto& [name, id] : sinkMap) {
          if (id == sinkId) return; // Was a memory sink, handled
        }
      }
      ofLogError("Synth") << "Float receive for unknown sinkId " << sinkId;
  }
}

void Synth::applyIntent(const Intent& intent, float intentStrength) {
  // Delegate memory bank intent application to controller
  memoryBankController->applyIntent(intent, intentStrength);
}

void Synth::update() {
  // Aggregate max auto agency from any .AgencyAuto connections.
  // This intentionally affects `getAgency()` on the next frame to avoid reliance on Mod update ordering.
  autoAgencyAggregateThisFrame = 0.0f;

  pauseStatus = paused ? "Yes" : "No";
#ifdef TARGET_MAC
  recorderStatus = (videoRecorderPtr && videoRecorderPtr->isRecording()) ? "Yes" : "No";
#else
  recorderStatus = "No";
#endif
  saveStatus = ofToString(getActiveSaveCount());
  
  // Update global ParamController settings from Synth parameters
  ParamControllerSettings::instance().manualBiasDecaySec = manualBiasDecaySecParameter;
  ParamControllerSettings::instance().baseManualBias = baseManualBiasParameter;
  
  // Update performance navigator hold state
  performanceNavigator.update();
  
  // Update hibernation fade even when paused
  hibernationController->update();
  
  // Update crossfade transition
  configTransitionManager->update();
  
  // Update per-layer pause envelopes
  layerController->updatePauseStates();
  
  // Sync paused state with hibernation:
  // - HIBERNATED: paused = true (fully asleep, nothing updates)
  // - FADING_OUT/FADING_IN: paused = false (mods update during transitions)
  // - ACTIVE: paused controlled by user (spacebar toggle)
  if (hibernationController->isFullyHibernated()) {
    paused = true;
  }
  
  // When paused and not in a fade transition, skip everything
  if (paused && !hibernationController->isFading()) {
    return;
  }
  
  // Ensure time tracking starts once the synth is actually running.
  // Previously this only started on first Spacebar wake-from-hibernation, which meant
  // `getSynthRunningTime()` could stay at 0 in always-active sessions.
  if (!paused && !timeTracker->hasEverRun()) {
    timeTracker->start();
  }

  // Accumulate running time when not paused
  if (!paused && timeTracker->hasEverRun()) {
    // Cap frame time to avoid time racing ahead during slow/unstable frames at startup
    // Use 2x target frame time (assuming 30fps target = 0.033s, cap at ~0.066s)
    float dt = std::min(ofGetLastFrameTime(), 0.066);
    timeTracker->accumulate(dt);
  }
  
  // Update Mods only when not paused
  if (!paused) {
    TS_START("Synth-updateIntents");
    intentController->update();
    applyIntentToAllMods();
    TS_STOP("Synth-updateIntents");

    backgroundColorController.update();
    
    layerController->clearActiveLayers(DEFAULT_CLEAR_COLOR);
    
    std::for_each(modPtrs.cbegin(), modPtrs.cend(), [](const auto& pair) {
      const auto& [name, modPtr] = pair;
      TSGL_START(name);
      TS_START(name);
      modPtr->update();
      TS_STOP(name);
      TSGL_STOP(name);
    });

    // Latch "register shift" events from any AgencyController.
    // This is used only for GUI signaling.
    int shiftCount = 0;
    int shiftIdCount = 0;
    for (const auto& [name, modPtr] : modPtrs) {
      if (auto agencyControllerPtr = std::dynamic_pointer_cast<AgencyControllerMod>(modPtr)) {
        if (agencyControllerPtr->wasTriggeredThisFrame()) {
          shiftCount++;
          if (shiftIdCount < MAX_AGENCY_REGISTER_SHIFT_IDS) {
            lastAgencyRegisterShiftIds[static_cast<size_t>(shiftIdCount)] = modPtr->getId();
            shiftIdCount++;
          }
        }
      }
    }
    if (shiftCount > 0) {
      lastAgencyRegisterShiftTimeSec = ofGetElapsedTimef();
      lastAgencyRegisterShiftCount = shiftCount;
      lastAgencyRegisterShiftIdCount = shiftIdCount;
    }

    autoAgencyAggregatePrev = autoAgencyAggregateThisFrame;
  }
  
  // Always update composites (whether paused or not) when hibernating
  // When not hibernating, only update if not paused
  TSGL_START("Synth-updateComposites");
  TS_START("Synth-updateComposites");
  
  // Phase 1: Update composite base layers
  CompositeRenderer::CompositeParams compositeParams {
    .layers = *layerController,
    .hibernationAlpha = hibernationController->getAlpha(),
    .backgroundColor = backgroundColorController.value,
    .backgroundBrightness = backgroundBrightnessParameter
  };
  compositeRenderer->updateCompositeBase(compositeParams);
  
  // Phase 2: Let mods render overlays (they sample the current composite)
  if (!paused) {
    for (const auto& [name, modPtr] : modPtrs) {
      modPtr->drawOverlay();
    }
  }
  
  // Phase 3: Composite overlay layers on top
  compositeRenderer->updateCompositeOverlays(compositeParams);
  
  // Update side panels
  compositeRenderer->updateSidePanels();
  
  TS_STOP("Synth-updateComposites");
  TSGL_STOP("Synth-updateComposites");
  
  // Process deferred manual image save immediately after composite is ready.
  // This timing ensures PBO bind happens while GPU is still working on this frame's data.
  // If the saver is busy, keep the manual request pending and retry next frame.
  if (pendingImageSave) {
    bool accepted = imageSaver->requestSave(compositeRenderer->getCompositeFbo(), pendingImageSavePath);
    if (accepted) {
      pendingImageSave = false;
      pendingImageSavePath.clear();
    }
  }

  // Auto-save full-res HDR composite snapshots (pre-tonemap, EXR).
  // Guards:
  // - Never during pause/hibernation
  // - No overlap: only when saver is fully idle
  if (AUTO_SNAPSHOTS_ENABLED &&
      !paused &&
      !currentConfigPath.empty() &&
      hibernationController && !hibernationController->isHibernating() &&
      imageSaver &&
      !pendingImageSave) {

    const std::string configId = getCurrentConfigId();
    if (!configId.empty()) {
      imageSaver->requestAutoSaveIfDue(
          compositeRenderer->getCompositeFbo(),
          getClockTimeSinceFirstRun(),
          AUTO_SNAPSHOTS_INTERVAL_SEC,
          AUTO_SNAPSHOTS_JITTER_SEC,
          [configId]() {
            std::string timestamp = ofGetTimestampString();
            return Synth::saveArtefactFilePath(
                AUTO_SNAPSHOTS_FOLDER_NAME + "/" + configId + "/drawing-" + timestamp + ".exr");
          });
    }
  }

  // Update memory bank controller (process pending saves, auto-capture, save-all requests)
  memoryBankController->update(compositeRenderer->getCompositeFbo(),
                               configRootPathSet ? configRootPath : std::filesystem::path{},
                               getSynthRunningTime());
  
  if (!paused) {
    emit(Synth::SOURCE_COMPOSITE_FBO, compositeRenderer->getCompositeFbo());
  }
}

void Synth::updateDebugViewFbo() {
  if (!debugViewEnabled) return;
  if (debugViewMode != DebugViewMode::Fbo) return;

  // Allocate FBO if needed
  if (!debugViewFbo.isAllocated()) {
    ofFboSettings settings;
    settings.width = DEBUG_VIEW_SIZE;
    settings.height = DEBUG_VIEW_SIZE;
    settings.internalformat = GL_RGBA;
    settings.useDepth = false;
    settings.useStencil = false;
    debugViewFbo.allocate(settings);
  }

  debugViewFbo.begin();
  ofClear(20, 20, 20, 255);

  // Draw Mod debug overlays in [0,1] normalized coordinates
  ofPushMatrix();
  ofPushStyle();
  ofScale(DEBUG_VIEW_SIZE, DEBUG_VIEW_SIZE);
  for (const auto& [name, modPtr] : modPtrs) {
    if (modPtr) {
      modPtr->draw();
    }
  }
  ofPopStyle();
  ofPopMatrix();

  debugViewFbo.end();
}

// Does not draw the GUI: see drawGui()
void Synth::draw() {
  TSGL_START("Synth::draw");
  compositeRenderer->draw(ofGetWindowWidth(), ofGetWindowHeight(),
                          displayController->getSettings(),
                          displayController->getSidePanelSettings(),
                          configTransitionManager.get());

  // Performer cues: draw in window space (not in composite, not in recordings/snapshots)
  if (cueGlyphController) {
    CueGlyphController::DrawParams cueParams;
    cueParams.audioEnabled = performerCues.audioEnabled;
    cueParams.videoEnabled = performerCues.videoEnabled;
    cueParams.alpha = displayController ? displayController->getCueAlpha().get() : 0.0f;

    constexpr int WARN_SEC = 10;
    auto& nav = performanceNavigator;
    if (nav.isConfigTimeExpired(ofGetElapsedTimef())) {
      cueParams.flashExpired = true;
    } else {
      cueParams.imminentConfigChangeProgress = nav.getImminentConfigChangeProgress(WARN_SEC);
    }

    cueGlyphController->draw(cueParams, ofGetWindowWidth(), ofGetWindowHeight());
  }

  updateDebugViewFbo();  // Render Mod debug draws to FBO for ImGui display
  TSGL_STOP("Synth::draw");

#ifdef TARGET_MAC
  // Capture frames for recording:
  // - During ACTIVE: capture if not paused
  // - During FADING_OUT/FADING_IN/HIBERNATED: always capture to stay in sync with audio
  // Recording captures the full audience experience including fades and black screen
  bool shouldCaptureFrame = videoRecorderPtr && videoRecorderPtr->isRecording() &&
      (!paused || hibernationController->isHibernating());
  
  if (shouldCaptureFrame) {
    TS_START("Synth::draw captureFrame");
    videoRecorderPtr->captureFrame([this](ofFbo& fbo) {
      compositeRenderer->drawToFbo(fbo, displayController->getSettings(),
                                   displayController->getSidePanelSettings(),
                                   configTransitionManager.get());
    });
    TS_STOP("Synth::draw captureFrame");
  }
#endif
  
  imageSaver->update();
}


void Synth::drawGui() {
  if (!guiVisible) return;
  gui.draw();
}

bool Synth::isRecording() const {
#ifdef TARGET_MAC
  return videoRecorderPtr && videoRecorderPtr->isRecording();
#else
  return false;
#endif
}

std::string Synth::getCurrentConfigId() const {
  if (currentConfigPath.empty()) {
    return {};
  }

  const std::filesystem::path p = currentConfigPath;
  if (!p.has_stem()) {
    return {};
  }
  return p.stem().string();
}

void Synth::toggleRecording() {
#ifdef TARGET_MAC
  if (!videoRecorderPtr) return;
  
  if (videoRecorderPtr->isRecording()) {
    videoRecorderPtr->stopRecording();
    
    if (audioAnalysisClientPtr) {
      audioAnalysisClientPtr->stopSegmentRecording();
    }
  } else {
    const std::string configId = getCurrentConfigId();
    if (configId.empty()) {
      ofLogError("Synth") << "toggleRecording: no config loaded";
      return;
    }

    std::string timestamp = ofGetTimestampString();
    std::string videoPath = Synth::saveArtefactFilePath(
        VIDEOS_FOLDER_NAME + "/" + configId + "/drawing-" + timestamp + ".mp4");
    
    // Start audio segment recording with matching timestamp
    if (audioAnalysisClientPtr) {
       std::string audioPath = Synth::saveArtefactFilePath(
           VIDEOS_FOLDER_NAME + "/" + configId + "/audio-" + timestamp + ".wav");

      audioAnalysisClientPtr->startSegmentRecording(audioPath);
    }
    
    videoRecorderPtr->startRecording(videoPath);
  }
#endif
}

void Synth::saveImage() {
  // Defer save to next update() - PBO bind happens right after composite is rendered
  // This avoids GPU stalls from binding PBO at arbitrary points in the frame
  const std::string configId = getCurrentConfigId();
  if (configId.empty()) {
    ofLogError("Synth") << "saveImage: no config loaded";
    return;
  }

  std::string timestamp = ofGetTimestampString();
  pendingImageSavePath = Synth::saveArtefactFilePath(
      SNAPSHOTS_FOLDER_NAME + "/" + configId + "/drawing-" + timestamp + ".exr");
  pendingImageSave = true;
}

void Synth::requestSaveAllMemories() {
  memoryBankController->requestSaveAll();
}

int Synth::getActiveSaveCount() const {
  return imageSaver ? imageSaver->getActiveSaveCount() : 0;
}

bool Synth::keyPressed(int key) {
  // Don't handle keyboard if ImGui is capturing text input
  if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureKeyboard) return false;
  
  if (performanceNavigator.keyPressed(key)) return true;
  
  if (key == OF_KEY_TAB) { guiVisible = not guiVisible; return true; }
  
  if (key == '?') { gui.toggleHelpWindow(); return true; }
  
  // Per-layer pause toggles (1-8 map to visible layers in order)
  if (key >= '1' && key <= '8') {
    int index = key - '1';
    const auto& pauseParams = layerController->getPauseParamPtrs();
    if (index < static_cast<int>(pauseParams.size())) {
      layerController->togglePause(index);
      return true;
    }
  }
  
  if (key == OF_KEY_SPACE) {
    // Spacebar: wake from hibernation (with fade-in), or toggle pause
    if (hibernationController->isHibernating()) {
      hibernationController->wake();  // Will reverse fade-out or start fade-in
      // Mods start updating during fade-in (paused stays false)
      paused = false;
      // Start time tracking on first ever wake
      if (!timeTracker->hasEverRun()) {
        timeTracker->start();
      }
    } else if (paused) {
      paused = false;
    } else {
      paused = true;
    }
    return true;
  }
  
  if (key == 'H') {
    // H key: hibernate (with fade-out), or reverse fade-in
    hibernationController->hibernate();  // Will start fade-out or reverse fade-in
    // Mods continue updating during fade-out so the visual keeps changing as it fades
    // paused will be set when fully hibernated (handled elsewhere or leave running during fade)
    return true;
  }

  if (key == 'S') {
    saveImage();
    return true;
  }

  if (key == 'R') {
    toggleRecording();
    return true;
  }

  if (key == 'D') {
    toggleDebugView();
    return true;
  }

  if (key == 'T') {
    // Uppercase T: toggle Audio Inspector mode inside Debug View.
    // This is synth-level so it works across configs and independent of mod names.
    debugViewEnabled = true;
    debugViewMode = (debugViewMode == DebugViewMode::AudioInspector) ? DebugViewMode::Fbo : DebugViewMode::AudioInspector;
    return true;
  }

  bool handled = std::any_of(modPtrs.cbegin(), modPtrs.cend(), [&key](const auto& pair) {
    const auto& [name, modPtr] = pair;
    return modPtr->keyPressed(key);
  });
  return handled;
}

bool Synth::keyReleased(int key) {
  // Don't handle keyboard if ImGui is capturing text input
  if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureKeyboard) return false;

  if (performanceNavigator.keyReleased(key)) return true;
  return false;
}

bool Synth::loadModSnapshotSlot(int slotIndex) {
  return gui.loadSnapshotSlot(slotIndex);
}

bool Synth::toggleLayerPauseSlot(int layerIndex) {
  const auto& pauseParams = layerController->getPauseParamPtrs();
  if (layerIndex < 0 || layerIndex >= static_cast<int>(pauseParams.size())) return false;

  layerController->togglePause(layerIndex);
  return true;
}

// Hibernation and time tracking accessors (delegate to controllers)

ofEvent<HibernationController::CompleteEvent>& Synth::getHibernationCompleteEvent() {
  return hibernationController->completeEvent;
}

HibernationController::State Synth::getHibernationState() const {
  return hibernationController->getState();
}

float Synth::getHibernationFadeDurationSec() const {
  return hibernationController->getFadeOutDurationParameter();
}

bool Synth::hasEverRun() const {
  return timeTracker->hasEverRun();
}

float Synth::getClockTimeSinceFirstRun() const {
  return timeTracker->getClockTimeSinceFirstRun();
}

float Synth::getSynthRunningTime() const {
  return timeTracker->getSynthRunningTime();
}

float Synth::getConfigRunningTime() const {
  return timeTracker->getConfigRunningTime();
}

int Synth::getConfigRunningMinutes() const {
  return timeTracker->getConfigRunningMinutes();
}

int Synth::getConfigRunningSeconds() const {
  return timeTracker->getConfigRunningSeconds();
}



void Synth::initParameters() {
  parameters.clear();
  
  parameters.add(agencyParameter);
  parameters.add(manualBiasDecaySecParameter);
  parameters.add(baseManualBiasParameter);
  parameters.add(backgroundColorParameter);
  parameters.add(backgroundBrightnessParameter);
  parameters.add(hibernationController->getFadeOutDurationParameter());
  parameters.add(hibernationController->getFadeInDurationParameter());
  parameters.add(configTransitionManager->getDelaySecParameter());
  parameters.add(configTransitionManager->getDurationParameter());
  // Expose delegated controller parameters in the Synth parameter group (flattened),
  // so they are editable in the node editor and configurable like other Mod params.
  if (memoryBankController) {
    memoryBankController->buildParameterGroup();
    addFlattenedParameterGroup(parameters, memoryBankController->getParameterGroup());
  }
  
  // initialise Mod parameters but do not add them to Synth parameters
  std::for_each(modPtrs.cbegin(), modPtrs.cend(), [this](const auto& pair) {
    const auto& [name, modPtr] = pair;
    ofParameterGroup& pg = modPtr->getParameterGroup();
    // for a non-imgui, the Mod parameters are added into the Synth parameter group in configureGui()
  });
}

void Synth::setIntentPresets(const std::vector<IntentPtr>& presets) {
  intentController->setPresets(presets);
}

void Synth::setIntentStrength(float value) {
  intentController->setStrength(value);
}

void Synth::setIntentActivation(size_t index, float value) {
  intentController->setActivation(index, value);
}

void Synth::applyIntentToAllMods() {
  const auto& intent = intentController->getActiveIntent();
  float effectiveStrength = intentController->getEffectiveStrength();
  
  applyIntent(intent, effectiveStrength);
  for (auto& kv : modPtrs) {
    kv.second->applyIntent(intent, effectiveStrength);
  }
}

bool Synth::loadFromConfig(const std::string& filepath) {
  ofLogNotice("Synth") << "Loading config from: " << filepath;

  static bool factoryInitialized = false;
  if (!factoryInitialized) {
    ModFactory::initializeBuiltinTypes();
    factoryInitialized = true;
  }

  bool success = SynthConfigSerializer::load(std::static_pointer_cast<Synth>(shared_from_this()), filepath, resources);

  if (success) {
    currentConfigPath = filepath;
    if (hibernationController) {
      hibernationController->setConfigId(getCurrentConfigId());
    }
    ofLogNotice("Synth") << "Successfully loaded config from: " << filepath;

    // Load global memories once (on first config load)
    if (configRootPathSet) {
      memoryBankController->loadGlobalMemories(configRootPath);
    }

  } else {
    ofLogError("Synth") << "Failed to load config from: " << filepath;
  }

  return success;
}

static void updateConfigObjectJson(nlohmann::ordered_json& configJson,
                                  const Mod::ParamValueMap& currentValues,
                                  const Mod::ParamValueMap& defaultValues) {
  // 1) Update any keys that already exist in the file.
  for (auto cfgIt = configJson.begin(); cfgIt != configJson.end(); ++cfgIt) {
    const std::string key = cfgIt.key();
    if (!key.empty() && key[0] == '_') continue;

    auto valIt = currentValues.find(key);
    if (valIt != currentValues.end()) {
      cfgIt.value() = valIt->second;
    }
  }

  // 2) Add missing keys only when non-default.
  for (const auto& [key, value] : currentValues) {
    if (!key.empty() && key[0] == '_') continue;
    if (configJson.contains(key)) continue;

    auto defIt = defaultValues.find(key);
    if (defIt != defaultValues.end() && value != defIt->second) {
      configJson[key] = value;
    }
  }
}

bool Synth::saveToCurrentConfig() {
  if (currentConfigPath.empty()) {
    ofLogError("Synth") << "saveToCurrentConfig: no config loaded";
    return false;
  }

  const std::filesystem::path filepath = currentConfigPath;
  if (!std::filesystem::exists(filepath)) {
    ofLogError("Synth") << "saveToCurrentConfig: file does not exist: " << filepath;
    return false;
  }

  try {
    nlohmann::ordered_json j;
    {
      std::ifstream inFile(filepath);
      if (!inFile.is_open()) {
        ofLogError("Synth") << "saveToCurrentConfig: failed to open for read: " << filepath;
        return false;
      }
      inFile >> j;
    }

    if (!j.contains("synth") || !j["synth"].is_object()) {
      j["synth"] = nlohmann::ordered_json::object();
    }

    // Save Synth-level parameters (same strategy as Mods: overwrite existing keys, add non-default missing keys).
    updateConfigObjectJson(j["synth"], getCurrentParameterValues(), getDefaultParameterValues());

    if (!j.contains("mods") || !j["mods"].is_object()) {
      ofLogError("Synth") << "saveToCurrentConfig: missing 'mods' object";
      return false;
    }

    for (auto& [modName, modJson] : j["mods"].items()) {
      if (!modName.empty() && modName[0] == '_') continue;
      if (!modJson.is_object()) continue;

      auto it = modPtrs.find(modName);
      if (it == modPtrs.end() || !it->second) continue;

      updateModConfigJson(modJson, it->second);
    }

    const std::filesystem::path tmpPath = filepath.string() + ".tmp";
    {
      std::ofstream outFile(tmpPath);
      if (!outFile.is_open()) {
        ofLogError("Synth") << "saveToCurrentConfig: failed to open for write: " << tmpPath;
        return false;
      }
      outFile << j.dump(2);
    }

    // Replace original file.
    std::error_code ec;
    std::filesystem::rename(tmpPath, filepath, ec);
    if (ec) {
      std::error_code removeEc;
      std::filesystem::remove(filepath, removeEc);
      ec.clear();
      std::filesystem::rename(tmpPath, filepath, ec);
    }
    if (ec) {
      ofLogError("Synth") << "saveToCurrentConfig: failed to replace file: " << ec.message();
      return false;
    }

    ofLogNotice("Synth") << "Saved config to: " << filepath;
    return true;
  } catch (const std::exception& e) {
    ofLogError("Synth") << "saveToCurrentConfig: exception: " << e.what();
    return false;
  }
}

void Synth::updateModConfigJson(nlohmann::ordered_json& modJson, const ModPtr& modPtr) {
  const auto currentValues = modPtr->getCurrentParameterValues();
  const auto& defaultValues = modPtr->getDefaultParameterValues();

  if (!modJson.contains("config") || !modJson["config"].is_object()) {
    modJson["config"] = nlohmann::ordered_json::object();
  }

  updateConfigObjectJson(modJson["config"], currentValues, defaultValues);
}

void Synth::switchToConfig(const std::string& filepath, bool useCrossfade) {
  // Capture snapshot before unload (if crossfading)
  if (useCrossfade) {
    configTransitionManager->captureSnapshot(compositeRenderer->getCompositeFbo());
  }
  
  // Emit unload event
  {
    ConfigUnloadEvent willEv;
    willEv.previousConfigPath = currentConfigPath;
    ofNotifyEvent(configWillUnloadEvent, willEv, this);
  }
  
  // Preserve per-Mod debug/UI state by mod name.
  captureModUiStateCache();

  // Preserve per-Mod runtime state by mod name.
  captureModRuntimeStateCache();

  // Unload and reload
  unload();
  
  // Reset config running time for the new config
  timeTracker->resetConfigTime();
  
  // Reset Synth-level parameters to defaults before loading new config,
  // so parameters not specified in the new config don't retain old values
  backgroundColorParameter.set(ofFloatColor { 0.0, 0.0, 0.0, 1.0 });
  backgroundBrightnessParameter.set(0.035f);
  
  bool loadOk = loadFromConfig(filepath);
  if (!loadOk) {
    ofLogError("Synth") << "switchToConfig: load failed, leaving Synth unloaded and paused";
    paused = true;  // Ensure no further mod activity
    configTransitionManager->cancelTransition();  // Cancel any pending transition
    return;
  }
  
  // Restore per-Mod runtime state after reload.
  restoreModRuntimeStateCache();

  // Restore per-Mod UI state after reload.
  restoreModUiStateCache();

  // Sync background color controller with newly loaded parameter value to prevent
  // old color bleeding through due to internal smoothing state
  backgroundColorController.syncWithParameter();
  
  // Note: Side panel timers and composite FBO clearing are managed by CompositeRenderer
  // The first updateCompositeBase() call will clear with the new background color
  
  initParameters();
  layerController->buildAlphaParameters();
  layerController->buildPauseParameters();
  gui.onConfigLoaded();
  
  // Emit load event
  {
    ConfigLoadedEvent didEv;
    didEv.newConfigPath = currentConfigPath;
    ofNotifyEvent(configDidLoadEvent, didEv, this);
  }
  
  ofLogNotice("Synth") << "Switched to config: " << currentConfigPath;
  
  // Begin crossfade transition
  if (useCrossfade) {
    configTransitionManager->beginTransition();
  }
}

void Synth::loadFirstPerformanceConfig() {
  performanceNavigator.loadFirstConfigIfAvailable();
}

std::optional<std::reference_wrapper<ofAbstractParameter>> Synth::findParameterByNamePrefix(const std::string& name) {
  if (auto ownParam = Mod::findParameterByNamePrefix(name)) {
    return ownParam;
  }

  for (auto& kv : modPtrs) {
    auto& modPtr = kv.second;
    if (!modPtr) continue;
    if (auto childParam = modPtr->findParameterByNamePrefix(name)) {
      return childParam;
    }
  }
  
  return std::nullopt;
}





} // ofxMarkSynth
