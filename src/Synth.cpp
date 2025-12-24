//
//  Synth.cpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#include "Synth.hpp"
#include "ofxTimeMeasurements.h"
#include "ofConstants.h"
#include "ofUtils.h"
#include "Gui.hpp"
#include "util/SynthConfigSerializer.hpp"
#include "util/TimeStringUtil.h"
#include "ofxImGui.h"
#include "ofxAudioAnalysisClient.h"
#include "nlohmann/json.hpp"
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
constexpr std::string VIDEOS_FOLDER_NAME = "drawing-recording";
// Also: camera-recording, mic-recording
// Also: ModSnapshotManager uses "mod-snapshots" and NodeEditorLayoutManager uses "node-layouts"


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
  if (synth && synth->pendingStartupConfigPath && !synth->pendingStartupConfigPath->empty()) {
    synth->loadFromConfig(*synth->pendingStartupConfigPath);
    synth->pendingStartupConfigPath.reset();
  }

  return synth;
}



Synth::Synth(const std::string& name_, ModConfig config, bool startHibernated, glm::vec2 compositeSize_, std::shared_ptr<ofxAudioAnalysisClient::LocalGistClient> audioAnalysisClient, ResourceManager resources_) :
Mod(nullptr, name_, std::move(config)),
paused { startHibernated },  // Start paused if hibernated
compositeSize { compositeSize_ },
resources { std::move(resources_) },
audioAnalysisClientPtr { std::move(audioAnalysisClient) },
hibernationState { startHibernated ? HibernationState::HIBERNATED : HibernationState::ACTIVE },
hibernationAlpha { startHibernated ? 0.0f : 1.0f }
{
  imageCompositeFbo.allocate(compositeSize.x, compositeSize.y, GL_RGB16F);
  compositeScale = std::min(ofGetWindowWidth() / imageCompositeFbo.getWidth(), ofGetWindowHeight() / imageCompositeFbo.getHeight());
  
  imageSaver = std::make_unique<AsyncImageSaver>(compositeSize);
  
  sidePanelWidth = (ofGetWindowWidth() - imageCompositeFbo.getWidth() * compositeScale) / 2.0 - *resources.get<float>("compositePanelGapPx");
  if (sidePanelWidth > 0.0) {
    sidePanelHeight = ofGetWindowHeight();
    leftPanelFbo.allocate(sidePanelWidth, sidePanelHeight, GL_RGB16F);
    rightPanelFbo.allocate(sidePanelWidth, sidePanelHeight, GL_RGB16F);
  }

  tonemapCrossfadeShader.load();

  crossfadeQuadMesh.setMode(OF_PRIMITIVE_TRIANGLE_FAN);
  crossfadeQuadMesh.getVertices() = {
    { 0.0f, 0.0f, 0.0f },
    { compositeSize.x, 0.0f, 0.0f },
    { compositeSize.x, compositeSize.y, 0.0f },
    { 0.0f, compositeSize.y, 0.0f },
  };
  crossfadeQuadMesh.getTexCoords() = {
    { 0.0f, 0.0f },
    { 1.0f, 0.0f },
    { 1.0f, 1.0f },
    { 0.0f, 1.0f },
  };

  unitQuadMesh.setMode(OF_PRIMITIVE_TRIANGLE_FAN);
  unitQuadMesh.getVertices() = {
    { 0.0f, 0.0f, 0.0f },
    { 1.0f, 0.0f, 0.0f },
    { 1.0f, 1.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f },
  };
  unitQuadMesh.getTexCoords() = {
    { 0.0f, 0.0f },
    { 1.0f, 0.0f },
    { 1.0f, 1.0f },
    { 0.0f, 1.0f },
  };
  
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
  
#ifdef TARGET_MAC
  videoRecorderPtr = std::make_unique<VideoRecorder>();
  videoRecorderPtr->setup(
      *resources.get<glm::vec2>("recorderCompositeSize"),
      *resources.get<std::filesystem::path>("ffmpegBinaryPath"));
#endif
  
  of::random::seed(0);
  
  // Initialize performance navigator from ResourceManager if path is provided (allow for the old manual configuration until that gets stripped and simplified)
  if (resources.has("performanceConfigRootPath")) {
    auto pathPtr = resources.get<std::filesystem::path>("performanceConfigRootPath");
    if (pathPtr) {
      performanceNavigator.loadFromFolder(*pathPtr / "synth");
    }
  }

  // Optional: pick a startup config by name (stem, not path) from the performance folder list.
  // This is meant for rapid iteration when developing performance config sequences.
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
  
  sourceNameIdMap = {
    { "CompositeFbo", SOURCE_COMPOSITE_FBO },
    { "Memory", SOURCE_MEMORY }
  };
  sinkNameIdMap = {
    { backgroundColorParameter.getName(), SINK_BACKGROUND_COLOR },
    { "ResetRandomness", SINK_RESET_RANDOMNESS },
    { "MemorySave", SINK_MEMORY_SAVE },
    { "MemorySaveSlot", SINK_MEMORY_SAVE_SLOT },
    { "MemoryEmit", SINK_MEMORY_EMIT },
    { "MemoryEmitSlot", SINK_MEMORY_EMIT_SLOT },
    { "MemoryEmitRandom", SINK_MEMORY_EMIT_RANDOM },
    { "MemoryEmitRandomNew", SINK_MEMORY_EMIT_RANDOM_NEW },
    { "MemoryEmitRandomOld", SINK_MEMORY_EMIT_RANDOM_OLD },
    { memorySaveCentreParameter.getName(), SINK_MEMORY_SAVE_CENTRE },
    { memorySaveWidthParameter.getName(), SINK_MEMORY_SAVE_WIDTH },
    { memoryEmitCentreParameter.getName(), SINK_MEMORY_EMIT_CENTRE },
    { memoryEmitWidthParameter.getName(), SINK_MEMORY_EMIT_WIDTH },
    { "MemoryClearAll", SINK_MEMORY_CLEAR_ALL }
  };
  
  // Allocate memory bank (1024x1024 fragments from the composite)
  memoryBank.allocate({ 1024, 1024 }, GL_RGBA8);
}

// TODO: fold this into loadFromConfig and the ctor?
void Synth::configureGui(std::shared_ptr<ofAppBaseWindow> windowPtr) {
  initDisplayParameterGroup();
  initFboParameterGroup();
  initLayerPauseParameterGroup();
  initMemoryBankParameterGroup();
  
  initIntentParameterGroup();
  
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
  drawingLayerPtrs.clear();
  initialLayerAlphas.clear();

  // 3) Clear GUI live texture hooks
  liveTexturePtrFns.clear();

  // 4) Clear parameter groups related to config
  layerAlphaParamPtrs.clear();
  layerAlphaParameters.clear();

  intentActivationParameters.clear();
  intentParameters.clear();
  intentActivations.clear();

  // 5) Clear current config path
  currentConfigPath.clear();

  // Note: Keep displayParameters, imageCompositeFbo, side panel FBOs, and tonemapCrossfadeShader
  // Rebuild of groups happens when reloading config (configureGui/init* called then)
}

DrawingLayerPtr Synth::addDrawingLayer(std::string name, glm::vec2 size, GLint internalFormat, int wrap, bool clearOnUpdate, ofBlendMode blendMode, bool useStencil, int numSamples, bool isDrawn, bool isOverlay, const std::string& description) {
  auto fboPtr = std::make_shared<PingPongFbo>();
  fboPtr->allocate(size, internalFormat, wrap, useStencil, numSamples);
  fboPtr->clearFloat(DEFAULT_CLEAR_COLOR);
  DrawingLayerPtr drawingLayerPtr = std::make_shared<DrawingLayer>(name, fboPtr, clearOnUpdate, blendMode, isDrawn, isOverlay, description);
  drawingLayerPtrs.insert({ name, drawingLayerPtr });
  return drawingLayerPtr;
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
  float now = ofGetElapsedTimef();
  
  auto emitMemoryWithRateLimit = [&](const ofTexture* tex, int sourceId) {
    if (!tex) return;
    if (now - lastMemoryEmitTime < memoryEmitMinIntervalParameter) return;
    lastMemoryEmitTime = now;
    emit(sourceId, *tex);
  };
  
  switch (sinkId) {
    case SINK_RESET_RANDOMNESS:
      {
        // Use bucketed onset value as seed for repeatability
        int seed = static_cast<int>(v * 10.0f); // Adjust multiplier for desired granularity
        of::random::seed(seed);
//        ofLogNotice("Synth") << "Reset seed: " << seed;
      }
      break;
      
    // Memory bank: save operations
    case SINK_MEMORY_SAVE:
      if (v > 0.5f) {
        memoryBank.save(imageCompositeFbo,
            memorySaveCentreController.value,
            memorySaveWidthController.value);
      }
      break;
      
    case SINK_MEMORY_SAVE_SLOT:
      {
        int slot = static_cast<int>(v) % MemoryBank::NUM_SLOTS;
        memoryBank.saveToSlot(imageCompositeFbo, slot);
//        ofLogNotice("Synth") << "Memory saved to slot " << slot;
      }
      break;
      
    // Memory bank: emit operations
    case SINK_MEMORY_EMIT:
      if (v > 0.5f) {
        emitMemoryWithRateLimit(
            memoryBank.select(memoryEmitCentreController.value,
                              memoryEmitWidthController.value),
            SOURCE_MEMORY);
      }
      break;
      
    case SINK_MEMORY_EMIT_SLOT:
      {
        int slot = static_cast<int>(v) % MemoryBank::NUM_SLOTS;
        emitMemoryWithRateLimit(memoryBank.get(slot), SOURCE_MEMORY);
      }
      break;
      
    case SINK_MEMORY_EMIT_RANDOM:
      if (v > 0.0f) {
        emitMemoryWithRateLimit(memoryBank.selectRandom(), SOURCE_MEMORY);
      }
      break;
      
    case SINK_MEMORY_EMIT_RANDOM_NEW:
      if (v > 0.5f) {
        emitMemoryWithRateLimit(
            memoryBank.selectWeightedRecent(memoryEmitCentreController.value,
                                             memoryEmitWidthController.value),
            SOURCE_MEMORY);
      }
      break;
      
    case SINK_MEMORY_EMIT_RANDOM_OLD:
      if (v > 0.5f) {
        emitMemoryWithRateLimit(
            memoryBank.selectWeightedOld(memoryEmitCentreController.value,
                                          memoryEmitWidthController.value),
            SOURCE_MEMORY);
      }
      break;
      
    // Memory bank: parameter updates
    case SINK_MEMORY_SAVE_CENTRE:
      memorySaveCentreController.updateAuto(v, getAgency());
      break;
      
    case SINK_MEMORY_SAVE_WIDTH:
      memorySaveWidthController.updateAuto(v, getAgency());
      break;
      
    case SINK_MEMORY_EMIT_CENTRE:
      memoryEmitCentreController.updateAuto(v, getAgency());
      break;
      
    case SINK_MEMORY_EMIT_WIDTH:
      memoryEmitWidthController.updateAuto(v, getAgency());
      break;
      
    case SINK_MEMORY_CLEAR_ALL:
      if (v > 0.5f) {
        memoryBank.clearAll();
        ofLogNotice("Synth") << "Memory bank cleared";
      }
      break;
      
    default:
      ofLogError("Synth") << "Float receive for unknown sinkId " << sinkId;
  }
}

void Synth::applyIntent(const Intent& intent, float intentStrength) {
  // Structure & Inverse Chaos -> background brightness
//  float structure = intent.getStructure();
//  float chaos = intent.getChaos();
//  float brightness = ofLerp(0.0f, 0.15f, structure) * (1.0f - chaos * 0.5f);
//  ofFloatColor target = ofFloatColor(brightness, brightness, brightness, 1.0f);
//  backgroundColorController.updateIntent(target, intentStrength);
  
  // Memory bank intent mappings
  // Chaos -> emit width (more chaos = wider/more random selection)
  float emitWidth = ofLerp(0.2f, 1.0f, intent.getChaos());
  memoryEmitWidthController.updateIntent(emitWidth, intentStrength);
  
  // Energy -> emit centre (high energy = recent memories)
  float emitCentre = ofLerp(0.3f, 0.9f, intent.getEnergy());
  memoryEmitCentreController.updateIntent(emitCentre, intentStrength);
  
  // Structure -> save width (more structure = more predictable/sequential saves)
  float saveWidth = ofLerp(0.5f, 0.0f, intent.getStructure());
  memorySaveWidthController.updateIntent(saveWidth, intentStrength);
}

void Synth::update() {
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
  updateHibernation();
  
  // Update crossfade transition
  updateTransition();
  
  // Update per-layer pause envelopes
  updateLayerPauseStates();
  
  // When paused, skip Mod updates but still update composites if hibernating
  if (paused && hibernationState == HibernationState::ACTIVE) {
    return;
  }
  
  // Accumulate running time when not paused and has ever run
  if (!paused && hasEverRun_) {
    // Cap frame time to avoid time racing ahead during slow/unstable frames at startup
    // Use 2x target frame time (assuming 30fps target = 0.033s, cap at ~0.066s)
    float dt = std::min(ofGetLastFrameTime(), 0.066);
    synthRunningTimeAccumulator_ += dt;
    configRunningTimeAccumulator_ += dt;
  }
  
  // Update Mods only when not paused
  if (!paused) {
    TS_START("Synth-updateIntents");
    updateIntentActivations();
    computeActiveIntent();
    applyIntentToAllMods();
    activeIntentInfoLabel1 = ofToString("E") + ofToString(activeIntent.getEnergy(), 2) +
                            " D" + ofToString(activeIntent.getDensity(), 2) +
                            " C" + ofToString(activeIntent.getChaos(), 2);
    activeIntentInfoLabel2 = ofToString("S") + ofToString(activeIntent.getStructure(), 2) +
                            " G" + ofToString(activeIntent.getGranularity(), 2);
    TS_STOP("Synth-updateIntents");

    backgroundColorController.update();
    
    std::for_each(drawingLayerPtrs.begin(), drawingLayerPtrs.end(), [this](auto& pair) {

      const auto& [name, fcptr] = pair;
      if (fcptr->clearOnUpdate && fcptr->pauseState != DrawingLayer::PauseState::PAUSED) {
        fcptr->fboPtr->getSource().begin();
        ofClear(DEFAULT_CLEAR_COLOR);
        fcptr->fboPtr->getSource().end();
      }
    });
    
    std::for_each(modPtrs.cbegin(), modPtrs.cend(), [](const auto& pair) {
      const auto& [name, modPtr] = pair;
      TSGL_START(name);
      TS_START(name);
      modPtr->update();
      TS_STOP(name);
      TSGL_STOP(name);
    });
  }
  
  // Always update composites (whether paused or not) when hibernating
  // When not hibernating, only update if not paused
  TSGL_START("Synth-updateComposites");
  TS_START("Synth-updateComposites");
  updateCompositeImage();
  updateSidePanels();
  TS_STOP("Synth-updateComposites");
  TSGL_STOP("Synth-updateComposites");
  
  // Process deferred image save immediately after composite is ready
  // This timing ensures PBO bind happens while GPU is still working on this frame's data
  if (pendingImageSave) {
    imageSaver->requestSave(imageCompositeFbo, pendingImageSavePath);
    pendingImageSave = false;
    pendingImageSavePath.clear();
  }
  
  // Process any deferred memory bank saves (requested from GUI)
  memoryBank.processPendingSave(imageCompositeFbo);

  if (memorySaveAllRequested) {
    memorySaveAllRequested = false;
    if (!configRootPathSet) {
      ofLogWarning("Synth") << "Cannot save global memory bank: config root not set";
    } else {
      const std::filesystem::path folder = configRootPath / "memory" / "global";
      memoryBank.saveAllToFolder(folder);
    }
  }
  
  if (!paused) {
    emit(Synth::SOURCE_COMPOSITE_FBO, imageCompositeFbo);
  }
}

glm::vec2 randomCentralRectOrigin(glm::vec2 rectSize, glm::vec2 bounds) {
  float x = ofRandom(bounds.x / 4.0, bounds.x * 3.0 / 4.0 - rectSize.x);
  float y = ofRandom(bounds.y / 4.0, bounds.y * 3.0 / 4.0 - rectSize.y);
  return { x, y };
}

// target is the outgoing image; source is the incoming one
void Synth::updateSidePanels() {
  if (sidePanelWidth <= 0.0) return;
  
  if (ofGetElapsedTimef() - leftSidePanelLastUpdate > leftSidePanelTimeoutSecs) {
    leftSidePanelLastUpdate = ofGetElapsedTimef();
    leftPanelFbo.swap();
    glm::vec2 leftPanelImageOrigin = randomCentralRectOrigin({ sidePanelWidth, sidePanelHeight }, { imageCompositeFbo.getWidth(), imageCompositeFbo.getHeight() });
    leftPanelFbo.getSource().begin();
    imageCompositeFbo.getTexture().drawSubsection(0.0, 0.0, sidePanelWidth, sidePanelHeight, leftPanelImageOrigin.x, leftPanelImageOrigin.y);
    leftPanelFbo.getSource().end();
  }

  if (ofGetElapsedTimef() - rightSidePanelLastUpdate > rightSidePanelTimeoutSecs) {
    rightSidePanelLastUpdate = ofGetElapsedTimef();
    rightPanelFbo.swap();
    glm::vec2 rightPanelImageOrigin = randomCentralRectOrigin({ sidePanelWidth, sidePanelHeight }, { imageCompositeFbo.getWidth(), imageCompositeFbo.getHeight() });
    rightPanelFbo.getSource().begin();
    imageCompositeFbo.getTexture().drawSubsection(0.0, 0.0, sidePanelWidth, sidePanelHeight, rightPanelImageOrigin.x, rightPanelImageOrigin.y);
    rightPanelFbo.getSource().end();
  }
}

void Synth::updateCompositeImage() {
  struct LayerInfo {
    DrawingLayerPtr layer;
    float finalAlpha;
  };
  std::vector<LayerInfo> baseLayers;
  std::vector<LayerInfo> overlayLayers;
  
  size_t i = 0;
  std::for_each(drawingLayerPtrs.cbegin(), drawingLayerPtrs.cend(), [this, &i, &baseLayers, &overlayLayers](const auto& pair) {
    const auto& [name, dlptr] = pair;
    if (!dlptr->isDrawn) return;
    float layerAlpha = layerAlphaParameters.getFloat(i);
    ++i;
    if (layerAlpha == 0.0f) return;
    
    float finalAlpha = layerAlpha;
    if (hibernationState != HibernationState::ACTIVE) {
      finalAlpha *= hibernationAlpha;
    }
    if (finalAlpha == 0.0f) return;
    
    if (dlptr->isOverlay) {
      overlayLayers.push_back({ dlptr, finalAlpha });
    } else {
      baseLayers.push_back({ dlptr, finalAlpha });
    }
  });
  
  // 1) Base composite (background + base layers)
  imageCompositeFbo.begin();
  {
    ofFloatColor backgroundColor = backgroundColorController.value;
    backgroundColor *= backgroundMultiplierParameter; backgroundColor.a = 1.0f;
    ofClear(backgroundColor);
    
    for (const auto& info : baseLayers) {
      ofEnableBlendMode(info.layer->blendMode);
      ofSetColor(ofFloatColor { 1.0f, 1.0f, 1.0f, info.finalAlpha });
      info.layer->fboPtr->draw(0, 0, imageCompositeFbo.getWidth(), imageCompositeFbo.getHeight());
    }
  }
  imageCompositeFbo.end();
  
  // 2) Overlay mods render into their own FBOs, sampling current composite
  if (!paused) {
    std::for_each(modPtrs.cbegin(), modPtrs.cend(), [](const auto& pair) {
      const auto& [name, modPtr] = pair;
      modPtr->drawOverlay();
    });
  }
  
  // 3) Composite overlay layers on top
  if (!overlayLayers.empty()) {
    imageCompositeFbo.begin();
    {
      for (const auto& info : overlayLayers) {
        ofEnableBlendMode(info.layer->blendMode);
        ofSetColor(ofFloatColor { 1.0f, 1.0f, 1.0f, info.finalAlpha });
        info.layer->fboPtr->draw(0, 0, imageCompositeFbo.getWidth(), imageCompositeFbo.getHeight());
      }
    }
    imageCompositeFbo.end();
  }
}

void Synth::drawSidePanels(float xleft, float xright, float w, float h) {
  const auto easeIn = [](float x) { return x * x * x; };

  const auto drawPanel = [&](PingPongFbo& panelFbo, float lastUpdateTime, float timeoutSecs, float x) {
    if (timeoutSecs <= 0.0f) return;

    float cycleElapsed = (ofGetElapsedTimef() - lastUpdateTime) / timeoutSecs;
    float alphaIn = easeIn(ofClamp(cycleElapsed, 0.0f, 1.0f));

    const auto& oldTexData = panelFbo.getTarget().getTexture().getTextureData();
    const auto& newTexData = panelFbo.getSource().getTexture().getTextureData();

    tonemapCrossfadeShader.begin(toneMapTypeParameter,
                                sideExposureParameter,
                                gammaParameter,
                                whitePointParameter,
                                contrastParameter,
                                saturationParameter,
                                brightnessParameter,
                                hueShiftParameter,
                                alphaIn,
                                oldTexData.bFlipTexture,
                                newTexData.bFlipTexture,
                                panelFbo.getTarget().getTexture(),
                                panelFbo.getSource().getTexture());

    ofPushMatrix();
    ofTranslate(x, 0.0f);
    ofScale(w, h);
    ofSetColor(255);
    unitQuadMesh.draw();
    ofPopMatrix();

    tonemapCrossfadeShader.end();
  };

  drawPanel(leftPanelFbo, leftSidePanelLastUpdate, leftSidePanelTimeoutSecs, xleft);
  drawPanel(rightPanelFbo, rightSidePanelLastUpdate, rightSidePanelTimeoutSecs, xright);
}

void Synth::drawMiddlePanel(float w, float h, float scale) {
  ofPushMatrix();
  ofTranslate((w - imageCompositeFbo.getWidth() * scale) / 2.0, (h - imageCompositeFbo.getHeight() * scale) / 2.0);
  ofScale(scale, scale);
  
  if (transitionState == TransitionState::CROSSFADING && transitionSnapshotFbo.isAllocated()) {
    const auto& snapshotTexData = transitionSnapshotFbo.getTexture().getTextureData();
    const auto& liveTexData = imageCompositeFbo.getTexture().getTextureData();

    tonemapCrossfadeShader.begin(toneMapTypeParameter,
                                exposureParameter,
                                gammaParameter,
                                whitePointParameter,
                                contrastParameter,
                                saturationParameter,
                                brightnessParameter,
                                hueShiftParameter,
                                transitionAlpha,
                                snapshotTexData.bFlipTexture,
                                liveTexData.bFlipTexture,
                                transitionSnapshotFbo.getTexture(),
                                imageCompositeFbo.getTexture());

    ofSetColor(255);
    crossfadeQuadMesh.draw();
    tonemapCrossfadeShader.end();
  } else {
    const auto& texData = imageCompositeFbo.getTexture().getTextureData();

    tonemapCrossfadeShader.begin(toneMapTypeParameter,
                                exposureParameter,
                                gammaParameter,
                                whitePointParameter,
                                contrastParameter,
                                saturationParameter,
                                brightnessParameter,
                                hueShiftParameter,
                                1.0f,
                                texData.bFlipTexture,
                                texData.bFlipTexture,
                                imageCompositeFbo.getTexture(),
                                imageCompositeFbo.getTexture());

    ofSetColor(255);
    crossfadeQuadMesh.draw();
    tonemapCrossfadeShader.end();
  }
  
  ofPopMatrix();
}

void Synth::updateDebugViewFbo() {
  if (!debugViewEnabled) return;
  
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
  ofClear(20, 20, 20, 200);  // Semi-transparent dark background for context
  
  // Draw Mod debug overlays in [0,1] normalized coordinates
  ofPushMatrix();
  ofPushStyle();
  ofScale(DEBUG_VIEW_SIZE, DEBUG_VIEW_SIZE);
  for (const auto& [name, modPtr] : modPtrs) {
    modPtr->draw();
  }
  ofPopStyle();
  ofPopMatrix();
  
  debugViewFbo.end();
}

// Does not draw the GUI: see drawGui()
void Synth::draw() {
  TSGL_START("Synth::draw");
  ofBlendMode(OF_BLENDMODE_DISABLED);
  drawSidePanels(0.0, ofGetWindowWidth() - sidePanelWidth, sidePanelWidth, sidePanelHeight);
  drawMiddlePanel(ofGetWindowWidth(), ofGetWindowHeight(), compositeScale);
  updateDebugViewFbo();  // Render Mod debug draws to FBO for ImGui display
  TSGL_STOP("Synth::draw");

#ifdef TARGET_MAC
  if (!paused && videoRecorderPtr && videoRecorderPtr->isRecording()) {
    TS_START("Synth::draw captureFrame");
    videoRecorderPtr->captureFrame([this](ofFbo& fbo) {
      float scale = fbo.getHeight() / imageCompositeFbo.getHeight();
      float panelWidth = (fbo.getWidth() - imageCompositeFbo.getWidth() * scale) / 2.0;
      drawSidePanels(0.0, fbo.getWidth() - panelWidth, panelWidth, sidePanelHeight);
      drawMiddlePanel(fbo.getWidth(), fbo.getHeight(), scale);
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

void Synth::toggleRecording() {
#ifdef TARGET_MAC
  if (!videoRecorderPtr) return;
  
  if (videoRecorderPtr->isRecording()) {
    videoRecorderPtr->stopRecording();
    
    if (audioAnalysisClientPtr) {
      audioAnalysisClientPtr->stopSegmentRecording();
    }
  } else {
    std::string timestamp = ofGetTimestampString();
    std::string videoPath = Synth::saveArtefactFilePath(
        VIDEOS_FOLDER_NAME + "/" + name + "/drawing-" + timestamp + ".mp4");
    
    // Start audio segment recording with matching timestamp
    if (audioAnalysisClientPtr) {
      std::string audioPath = Synth::saveArtefactFilePath(
          VIDEOS_FOLDER_NAME + "/" + name + "/audio-" + timestamp + ".wav");
      audioAnalysisClientPtr->startSegmentRecording(audioPath);
    }
    
    videoRecorderPtr->startRecording(videoPath);
  }
#endif
}

void Synth::saveImage() {
  // Defer save to next update() - PBO bind happens right after composite is rendered
  // This avoids GPU stalls from binding PBO at arbitrary points in the frame
  std::string timestamp = ofGetTimestampString();
  pendingImageSavePath = Synth::saveArtefactFilePath(
      SNAPSHOTS_FOLDER_NAME + "/" + name + "/drawing-" + timestamp + ".exr");
  pendingImageSave = true;
}

void Synth::requestSaveAllMemories() {
  memorySaveAllRequested = true;
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
    if (index < static_cast<int>(layerPauseParamPtrs.size())) {
      auto& p = layerPauseParamPtrs[index];
      p->set(!p->get());
      return true;
    }
  }
  
  if (key == OF_KEY_SPACE) {
    // Spacebar: unhibernate if hibernated/fading, unpause if paused, pause if running
    if (hibernationState != HibernationState::ACTIVE) {
      cancelHibernation();  // Also sets paused = false
    } else if (paused) {
      paused = false;
    } else {
      paused = true;
    }
    return true;
  }
  
  if (key == 'H') {
    // H key: hibernate only (one-way, not a toggle)
    if (hibernationState == HibernationState::ACTIVE) {
      startHibernation();
    }
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
  if (layerIndex < 0 || layerIndex >= static_cast<int>(layerPauseParamPtrs.size())) return false;

  auto& p = layerPauseParamPtrs[layerIndex];
  p->set(!p->get());
  return true;
}

// Hibernation system implementation

void Synth::startHibernation() {
  if (hibernationState == HibernationState::ACTIVE) {
    ofLogNotice("Synth") << "Starting hibernation, fade duration: "
                         << hibernationFadeDurationParameter.get() << "s";
    hibernationState = HibernationState::FADING_OUT;
    hibernationStartTime = ofGetElapsedTimef();
    paused = true;  // Pause updates immediately
  }
}

void Synth::cancelHibernation() {
  if (hibernationState == HibernationState::FADING_OUT) {
    ofLogNotice("Synth") << "Cancelling hibernation";
  }
  if (hibernationState != HibernationState::ACTIVE) {
    hibernationState = HibernationState::ACTIVE;
    hibernationAlpha = 1.0f;
    paused = false;  // Resume updates
    
    // Start all time tracking on first ever cancelHibernation
    if (!hasEverRun_) {
      hasEverRun_ = true;
      worldTimeAtFirstRun_ = ofGetElapsedTimef();
      synthRunningTimeAccumulator_ = 0.0f;
      configRunningTimeAccumulator_ = 0.0f;
      ofLogNotice("Synth") << "First run - all time tracking started";
    }
  }
}

void Synth::updateHibernation() {
  if (hibernationState != HibernationState::FADING_OUT) return;
  
  float elapsed = ofGetElapsedTimef() - hibernationStartTime;
  float duration = hibernationFadeDurationParameter.get();
  
  if (elapsed >= duration) {
    hibernationAlpha = 0.0f;
    hibernationState = HibernationState::HIBERNATED;
    
    ofLogNotice("Synth") << "Hibernation complete after " << elapsed << "s";
    HibernationCompleteEvent event;
    event.fadeDuration = elapsed;
    event.synthName = name;
    ofNotifyEvent(hibernationCompleteEvent, event, this);
  } else {
    float t = elapsed / duration;
    hibernationAlpha = 1.0f - t;
  }
}

std::string Synth::getHibernationStateString() const {
  switch (hibernationState) {
    case HibernationState::ACTIVE:     return "Active";
    case HibernationState::FADING_OUT: return "Hibernating...";
    case HibernationState::HIBERNATED: return "Hibernated";
  }
  return "Unknown";
}

// Config transition crossfade system

void Synth::captureSnapshot() {
  float w = imageCompositeFbo.getWidth();
  float h = imageCompositeFbo.getHeight();
  
  // Allocate snapshot FBO if needed
  if (!transitionSnapshotFbo.isAllocated() ||
      transitionSnapshotFbo.getWidth() != w ||
      transitionSnapshotFbo.getHeight() != h) {
    transitionSnapshotFbo.allocate(w, h, GL_RGB16F);
  }
  
  transitionSnapshotFbo.begin();
  {
    ofClear(0, 0, 0, 255);
    ofSetColor(255);
    imageCompositeFbo.draw(0, 0);
  }
  transitionSnapshotFbo.end();
}

void Synth::updateTransition() {
  if (transitionState != TransitionState::CROSSFADING) return;
  
  float elapsed = ofGetElapsedTimef() - transitionStartTime;
  float duration = crossfadeDurationParameter.get();
  
  if (elapsed >= duration) {
    transitionAlpha = 1.0f;
    transitionState = TransitionState::NONE;
  } else {
    float t = elapsed / duration;
    transitionAlpha = glm::smoothstep(0.0f, 1.0f, t);
  }
}

void Synth::initFboParameterGroup() {
  layerAlphaParameters.clear();
  layerAlphaParamPtrs.clear();
  
  layerAlphaParameters.setName("Layers");
  std::for_each(drawingLayerPtrs.cbegin(), drawingLayerPtrs.cend(), [this](const auto& pair) {
    const auto& [name, fcptr] = pair;
    if (!fcptr->isDrawn) return;
    float initialAlpha = 1.0f;
    auto it = initialLayerAlphas.find(fcptr->name);
    if (it != initialLayerAlphas.end()) {
      initialAlpha = it->second;
    }
    auto fboParam = std::make_shared<ofParameter<float>>(fcptr->name, initialAlpha, 0.0f, 1.0f);
    layerAlphaParamPtrs.push_back(fboParam);
    layerAlphaParameters.add(*fboParam);
  });
}

void Synth::initLayerPauseParameterGroup() {
  layerPauseParameters.clear();
  layerPauseParamPtrs.clear();
  
  layerPauseParameters.setName("Layer Pauses");
  std::for_each(drawingLayerPtrs.cbegin(), drawingLayerPtrs.cend(), [this](const auto& pair) {
    const auto& [name, layerPtr] = pair;
    if (!layerPtr->isDrawn) return;
    bool initialPaused = false;
    auto it = initialLayerPaused.find(layerPtr->name);
    if (it != initialLayerPaused.end()) {
      initialPaused = it->second;
    }
    auto p = std::make_shared<ofParameter<bool>>(layerPtr->name + " Paused", initialPaused);
    layerPauseParamPtrs.push_back(p);
    layerPauseParameters.add(*p);
    
    layerPtr->pauseState = initialPaused ? DrawingLayer::PauseState::PAUSED
                                         : DrawingLayer::PauseState::ACTIVE;
  });
}

void Synth::initIntentParameterGroup() {
  intentParameters.clear();

  intentParameters.setName("Intent");

  // Don't recreate intents here - they should be set via setIntentPresets() or initIntentPresets()
  // Just add the existing activation parameters to the group
  for (auto& p : intentActivationParameters) intentParameters.add(*p);

  // Keep master intent strength at the end so it appears rightmost in the GUI.
  intentParameters.add(intentStrengthParameter);
}

void Synth::initDisplayParameterGroup() {
  displayParameters.clear();
  
  displayParameters.setName("Display");
  displayParameters.add(toneMapTypeParameter);
  displayParameters.add(exposureParameter);
  displayParameters.add(gammaParameter);
  displayParameters.add(whitePointParameter);
  displayParameters.add(contrastParameter);
  displayParameters.add(saturationParameter);
  displayParameters.add(brightnessParameter);
  displayParameters.add(hueShiftParameter);
  displayParameters.add(sideExposureParameter);
}

void Synth::updateLayerPauseStates() {
  // Build lookup: base layer name -> pause parameter
  std::unordered_map<std::string, ofParameter<bool>*> pauseByName;
  for (auto& p : layerPauseParamPtrs) {
    std::string baseName = p->getName();
    auto pos = baseName.rfind(" Paused");
    if (pos != std::string::npos) {
      baseName = baseName.substr(0, pos);
    }
    pauseByName[baseName] = p.get();
  }
  
  for (auto& [name, layerPtr] : drawingLayerPtrs) {
    if (!layerPtr->isDrawn) continue;
    
    bool targetPaused = false;
    if (auto it = pauseByName.find(name); it != pauseByName.end()) {
      targetPaused = it->second->get();
    }
    
    layerPtr->pauseState = targetPaused ? DrawingLayer::PauseState::PAUSED
                                        : DrawingLayer::PauseState::ACTIVE;
  }
}

void Synth::initMemoryBankParameterGroup() {
  memoryBankParameters.clear();
  
  memoryBankParameters.setName("MemoryBank");
  memoryBankParameters.add(memorySaveCentreParameter);
  memoryBankParameters.add(memorySaveWidthParameter);
  memoryBankParameters.add(memoryEmitCentreParameter);
  memoryBankParameters.add(memoryEmitWidthParameter);
  memoryBankParameters.add(memoryEmitMinIntervalParameter);
}

void Synth::initParameters() {
  parameters.clear();
  
  parameters.add(agencyParameter);
  parameters.add(manualBiasDecaySecParameter);
  parameters.add(baseManualBiasParameter);
  parameters.add(backgroundColorParameter);
  parameters.add(backgroundMultiplierParameter);
  parameters.add(hibernationFadeDurationParameter);
  parameters.add(crossfadeDurationParameter);
  
  // initialise Mod parameters but do not add them to Synth parameters
  std::for_each(modPtrs.cbegin(), modPtrs.cend(), [this](const auto& pair) {
    const auto& [name, modPtr] = pair;
    ofParameterGroup& pg = modPtr->getParameterGroup();
    // for a non-imgui, the Mod parameters are added into the Synth parameter group in configureGui()
  });
}

void Synth::setIntentPresets(const std::vector<IntentPtr>& presets) {
  if (presets.size() > 7) {
    ofLogWarning("Synth") << "Received " << presets.size() << " intents, limiting to 7";
  }
  
  intentActivations.clear();
  intentActivationParameters.clear();
  
  size_t count = std::min(presets.size(), size_t(7));
  for (size_t i = 0; i < count; ++i) {
    intentActivations.emplace_back(presets[i]);
    auto p = std::make_shared<ofParameter<float>>(presets[i]->getName() + " Activation", 0.0f, 0.0f, 1.0f);
    intentActivationParameters.push_back(p);
  }
  
  ofLogNotice("Synth") << "Set " << count << " intent presets from config";
}

void Synth::setIntentStrength(float value) {
    intentStrengthParameter.set(ofClamp(value, 0.0f, 1.0f));
}

void Synth::setIntentActivation(size_t index, float value) {
    if (index >= intentActivations.size()) {
        ofLogWarning("Synth") << "setIntentActivation: index " << index
                              << " out of range (have " << intentActivations.size() << " intents)";
        return;
    }
    float clamped = ofClamp(value, 0.0f, 1.0f);
    intentActivations[index].activation = clamped;
    intentActivations[index].targetActivation = clamped;
    if (index < intentActivationParameters.size()) {
        intentActivationParameters[index]->set(clamped);
    }
}

void Synth::updateIntentActivations() {
  float dt = ofGetLastFrameTime();
  for (size_t i = 0; i < intentActivations.size(); ++i) {
    auto& ia = intentActivations[i];
    float target = intentActivationParameters[i]->get();
    ia.targetActivation = target;
    float speed = ia.transitionSpeed;
    float alpha = 1.0f - expf(-dt * std::max(0.001f, speed) * 4.0f);
    ia.activation = ofLerp(ia.activation, ia.targetActivation, alpha);
  }
}

void Synth::computeActiveIntent() {
  static std::vector<std::pair<IntentPtr, float>> weighted;
  if (weighted.size() != intentActivations.size()) {
    weighted.clear();
    weighted.resize(intentActivations.size());
  }
  for (size_t i = 0; i < intentActivations.size(); ++i) {
    auto& ia = intentActivations[i];
    weighted[i].first = ia.intentPtr;
    weighted[i].second = ia.activation;
  }
  activeIntent.setWeightedBlend(weighted);
}

void Synth::applyIntentToAllMods() {
  // Scale effective intent strength by total activation weight
  float totalActivation = 0.0f;
  for (const auto& ia : intentActivations) {
    totalActivation += ia.activation;
  }
  float effectiveStrength = intentStrengthParameter * std::min(1.0f, totalActivation);
  
  applyIntent(activeIntent, effectiveStrength);
  for (auto& kv : modPtrs) {
    kv.second->applyIntent(activeIntent, effectiveStrength);
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
    ofLogNotice("Synth") << "Successfully loaded config from: " << filepath;

    if (!globalMemoryBankLoaded) {
      if (!configRootPathSet) {
        ofLogWarning("Synth") << "Cannot load global memory bank: config root not set";
      } else {
        const std::filesystem::path folder = configRootPath / "memory" / "global";
        memoryBank.loadAllFromFolder(folder);
        globalMemoryBankLoaded = true;
      }
    }
  } else {
    ofLogError("Synth") << "Failed to load config from: " << filepath;
  }
  
  return success;
}

bool Synth::saveModsToCurrentConfig() {
  if (currentConfigPath.empty()) {
    ofLogError("Synth") << "saveModsToCurrentConfig: no config loaded";
    return false;
  }

  const std::filesystem::path filepath = currentConfigPath;
  if (!std::filesystem::exists(filepath)) {
    ofLogError("Synth") << "saveModsToCurrentConfig: file does not exist: " << filepath;
    return false;
  }

  try {
    nlohmann::ordered_json j;
    {
      std::ifstream inFile(filepath);
      if (!inFile.is_open()) {
        ofLogError("Synth") << "saveModsToCurrentConfig: failed to open for read: " << filepath;
        return false;
      }
      inFile >> j;
    }

    if (!j.contains("mods") || !j["mods"].is_object()) {
      ofLogError("Synth") << "saveModsToCurrentConfig: missing 'mods' object";
      return false;
    }

    for (auto& [modName, modJson] : j["mods"].items()) {
      if (!modName.empty() && modName[0] == '_') {
        continue;
      }

      if (!modJson.is_object()) {
        continue;
      }

      auto it = modPtrs.find(modName);
      if (it == modPtrs.end() || !it->second) {
        continue;
      }

      const ModPtr& modPtr = it->second;
      const auto currentValues = modPtr->getCurrentParameterValues();
      const auto& defaultValues = modPtr->getDefaultParameterValues();

      if (!modJson.contains("config") || !modJson["config"].is_object()) {
        modJson["config"] = nlohmann::ordered_json::object();
      }

      nlohmann::ordered_json& configJson = modJson["config"];

      // 1) Update any keys that already exist in the file.
      for (auto cfgIt = configJson.begin(); cfgIt != configJson.end(); ++cfgIt) {
        const std::string key = cfgIt.key();
        if (!key.empty() && key[0] == '_') {
          continue;
        }

        auto valIt = currentValues.find(key);
        if (valIt != currentValues.end()) {
          cfgIt.value() = valIt->second;
        }
      }

      // 2) Add missing keys only when non-default.
      for (const auto& [key, value] : currentValues) {
        if (!key.empty() && key[0] == '_') {
          continue;
        }

        if (configJson.contains(key)) {
          continue;
        }

        auto defIt = defaultValues.find(key);
        if (defIt == defaultValues.end()) {
          continue;
        }

        if (value != defIt->second) {
          configJson[key] = value;
        }
      }
    }

    const std::filesystem::path tmpPath = filepath.string() + ".tmp";
    {
      std::ofstream outFile(tmpPath);
      if (!outFile.is_open()) {
        ofLogError("Synth") << "saveModsToCurrentConfig: failed to open for write: " << tmpPath;
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
      ofLogError("Synth") << "saveModsToCurrentConfig: failed to replace file: " << ec.message();
      return false;
    }

    ofLogNotice("Synth") << "Saved mod params to config: " << filepath;
    return true;
  } catch (const std::exception& e) {
    ofLogError("Synth") << "saveModsToCurrentConfig: exception: " << e.what();
    return false;
  }
}

void Synth::switchToConfig(const std::string& filepath, bool useCrossfade) {
  // Capture snapshot before unload (if crossfading)
  if (useCrossfade) {
    captureSnapshot();
  }
  
  // Emit unload event
  {
    ConfigUnloadEvent willEv;
    willEv.previousConfigPath = currentConfigPath;
    ofNotifyEvent(configWillUnloadEvent, willEv, this);
  }
  
  // Unload and reload
  unload();
  
  // Reset config running time for the new config
  resetConfigRunningTime();
  
  // Reset Synth-level parameters to defaults before loading new config,
  // so parameters not specified in the new config don't retain old values
  backgroundColorParameter.set(ofFloatColor { 0.0, 0.0, 0.0, 1.0 });
  backgroundMultiplierParameter.set(0.1f);
  
  bool loadOk = loadFromConfig(filepath);
  if (!loadOk) {
    ofLogError("Synth") << "switchToConfig: load failed, leaving Synth unloaded and paused";
    paused = true;  // Ensure no further mod activity
    transitionState = TransitionState::NONE;  // Cancel any pending transition
    return;
  }
  
  // Sync background color controller with newly loaded parameter value to prevent
  // old color bleeding through due to internal smoothing state
  backgroundColorController.syncWithParameter();
  
  // Reset side panel update timers so they capture fresh content from the new config
  // immediately rather than showing stale content from the old config
  leftSidePanelLastUpdate = 0.0f;
  rightSidePanelLastUpdate = 0.0f;
  
  // Clear the composite FBO with the new background color to prevent old content
  // from showing through before the first frame renders
  if (imageCompositeFbo.isAllocated()) {
    imageCompositeFbo.begin();
    ofFloatColor bgColor = backgroundColorParameter.get();
    bgColor *= backgroundMultiplierParameter;
    bgColor.a = 1.0f;
    ofClear(bgColor);
    imageCompositeFbo.end();
  }
  
  initParameters();
  initFboParameterGroup();
  initLayerPauseParameterGroup();
  initIntentParameterGroup();
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
    transitionState = TransitionState::CROSSFADING;
    transitionStartTime = ofGetElapsedTimef();
    transitionAlpha = 0.0f;
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

// Time tracking implementations

float Synth::getClockTimeSinceFirstRun() const {
  if (!hasEverRun_) return 0.0f;
  return ofGetElapsedTimef() - worldTimeAtFirstRun_;
}

float Synth::getSynthRunningTime() const {
  return synthRunningTimeAccumulator_;
}

float Synth::getConfigRunningTime() const {
  return configRunningTimeAccumulator_;
}

int Synth::getConfigRunningMinutes() const {
  return static_cast<int>(configRunningTimeAccumulator_) / 60;
}

int Synth::getConfigRunningSeconds() const {
  return static_cast<int>(configRunningTimeAccumulator_) % 60;
}

void Synth::resetConfigRunningTime() {
  configRunningTimeAccumulator_ = 0.0f;
}



} // ofxMarkSynth
