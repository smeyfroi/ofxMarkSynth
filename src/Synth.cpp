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
#include "ofxImGui.h"
#include "sourceMods/AudioDataSourceMod.hpp"



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



std::shared_ptr<Synth> Synth::create(const std::string& name, ModConfig config, bool startPaused, glm::vec2 compositeSize, ResourceManager resources) {
  return std::shared_ptr<Synth>(
      new Synth(name,
                std::move(config),
                startPaused,
                compositeSize,
                std::move(resources)));
}



Synth::Synth(const std::string& name_, ModConfig config, bool startPaused, glm::vec2 compositeSize_, ResourceManager resources_) :
Mod(nullptr, name_, std::move(config)),
paused { startPaused },
compositeSize { compositeSize_ },
resources { std::move(resources_) }
{
  imageCompositeFbo.allocate(compositeSize.x, compositeSize.y, GL_RGB16F);
  compositeScale = std::min(ofGetWindowWidth() / imageCompositeFbo.getWidth(), ofGetWindowHeight() / imageCompositeFbo.getHeight());
  
  // Pre-allocate PBO for async image saving (RGB float, same size as composite)
  size_t pboSize = static_cast<size_t>(compositeSize.x) * static_cast<size_t>(compositeSize.y) * 3 * sizeof(float);
  imageSavePbo.allocate(pboSize, GL_STREAM_READ);
  
  sidePanelWidth = (ofGetWindowWidth() - imageCompositeFbo.getWidth() * compositeScale) / 2.0 - *resources.get<float>("compositePanelGapPx");
  if (sidePanelWidth > 0.0) {
    sidePanelHeight = ofGetWindowHeight();
    leftPanelFbo.allocate(sidePanelWidth, sidePanelHeight, GL_RGB16F);
    leftPanelCompositeFbo.allocate(sidePanelWidth, sidePanelHeight, GL_RGB16F);
    rightPanelFbo.allocate(sidePanelWidth, sidePanelHeight, GL_RGB16F);
    rightPanelCompositeFbo.allocate(sidePanelWidth, sidePanelHeight, GL_RGB16F);
  }

  tonemapShader.load();
  
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
  recorderCompositeSize = *resources.get<glm::vec2>("recorderCompositeSize");
  recorderCompositeFbo.allocate(recorderCompositeSize.x, recorderCompositeSize.y, GL_RGB);
  recorder.setup(/*video*/true, /*audio*/false, recorderCompositeFbo.getSize(), /*fps*/30.0, /*bitrate*/8000);
  recorder.setOverWrite(true);
  recorder.setFFmpegPath(resources.get<std::filesystem::path>("ffmpegBinaryPath")->string());
//  recorder.setInputPixelFormat(ofPixelFormat::OF_PIXELS_RGB);
  recorder.setVideoCodec("h264_videotoolbox"); // hw accelerated in macos. hevc_videotoolbox is higher quality but slower and less compatible
  
  // Allocate PBOs for async pixel readback
  size_t recorderPboSize = recorderCompositeSize.x * recorderCompositeSize.y * 3;  // RGB bytes
  for (int i = 0; i < NUM_PBOS; i++) {
    recorderPbos[i].allocate(recorderPboSize, GL_DYNAMIC_READ);
  }
  recorderPixels.allocate(recorderCompositeSize.x, recorderCompositeSize.y, OF_PIXELS_RGB);
#endif
  
  of::random::seed(0);
  
  // Initialize performance navigator from ResourceManager if path is provided (allow for the old manual configuration until that gets stripped and simplified)
  if (resources.has("performanceConfigRootPath")) {
    auto pathPtr = resources.get<std::filesystem::path>("performanceConfigRootPath");
    if (pathPtr) {
      performanceNavigator.loadFromFolder(*pathPtr/"synth");
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

void Synth::pruneSaveThreads() {
  saveToFileThreads.erase(
      std::remove_if(saveToFileThreads.begin(), saveToFileThreads.end(),
                     [](const std::unique_ptr<SaveToFileThread>& thread) {
                       return !thread->isThreadRunning();
                     }),
      saveToFileThreads.end());
}

void Synth::shutdown() {
  ofLogNotice("Synth") << "Synth::shutdown " << name;
  
  std::for_each(modPtrs.cbegin(), modPtrs.cend(), [](const auto& pair) {
    const auto& [name, modPtr] = pair;
    modPtr->shutdown();
  });
  
  gui.exit();
  
#ifdef TARGET_MAC
  if (recorder.isRecording()) {
    ofLogNotice("Synth") << "Stopping recording";
    recorder.stop();
    ofLogNotice("Synth") << "Recording stopped";
  }
#endif
  
  // Complete any pending PBO image save before waiting for threads
  if (pendingImageSave) {
    ofLogNotice("Synth") << "Waiting for pending image save PBO transfer";
    glClientWaitSync(pendingImageSave->fence, GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
    
    // Generate filepath now (was deferred from saveImage)
    std::string filepath = Synth::saveArtefactFilePath(
      SNAPSHOTS_FOLDER_NAME+"/"+name+"/drawing-"+pendingImageSave->timestamp+".exr");
    ofLogNotice("Synth") << "Saving drawing to " << filepath;
    
    int w = imageCompositeFbo.getWidth();
    int h = imageCompositeFbo.getHeight();
    size_t size = static_cast<size_t>(w) * static_cast<size_t>(h) * 3 * sizeof(float);
    
    ofFloatPixels pixels;
    pixels.allocate(w, h, OF_IMAGE_COLOR);
    
    imageSavePbo.bind(GL_PIXEL_PACK_BUFFER);
    void* ptr = imageSavePbo.map(GL_READ_ONLY);
    if (ptr) {
      memcpy(pixels.getData(), ptr, size);
      imageSavePbo.unmap();
    }
    imageSavePbo.unbind(GL_PIXEL_PACK_BUFFER);
    
    glDeleteSync(pendingImageSave->fence);
    
    pruneSaveThreads();
    auto threadPtr = std::make_unique<SaveToFileThread>();
    threadPtr->save(filepath, std::move(pixels));
    saveToFileThreads.push_back(std::move(threadPtr));
    
    pendingImageSave.reset();
    ofLogNotice("Synth") << "Pending image save handed off to thread";
  }
  
  std::for_each(saveToFileThreads.begin(), saveToFileThreads.end(), [](auto& thread) {
    ofLogNotice("Synth") << "Waiting for save thread to finish";
    thread->waitForThread(false);
    ofLogNotice("Synth") << "Done waiting for save thread to finish";
  });
  saveToFileThreads.clear();
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
  fboParamPtrs.clear();
  fboParameters.clear();

  intentActivationParameters.clear();
  intentParameters.clear();
  intentActivations.clear();

  // 5) Clear current config path
  currentConfigPath.clear();

  // Note: Keep displayParameters, imageCompositeFbo, side panel FBOs, and tonemapShader
  // Rebuild of groups happens when reloading config (configureGui/init* called then)
}

DrawingLayerPtr Synth::addDrawingLayer(std::string name, glm::vec2 size, GLint internalFormat, int wrap, bool clearOnUpdate, ofBlendMode blendMode, bool useStencil, int numSamples, bool isDrawn, bool isOverlay) {
  auto fboPtr = std::make_shared<PingPongFbo>();
  fboPtr->allocate(size, internalFormat, wrap, useStencil, numSamples);
  fboPtr->clearFloat(DEFAULT_CLEAR_COLOR);
  DrawingLayerPtr drawingLayerPtr = std::make_shared<DrawingLayer>(name, fboPtr, clearOnUpdate, blendMode, isDrawn, isOverlay);
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
  recorderStatus = recorder.isRecording() ? "Yes" : "No";
  saveStatus = ofToString(SaveToFileThread::getActiveThreadCount());
  
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
  updateCompositeSideImages();
  updateSidePanels();
  TS_STOP("Synth-updateComposites");
  TSGL_STOP("Synth-updateComposites");
  
  // Process any deferred memory bank saves (requested from GUI)
  memoryBank.processPendingSave(imageCompositeFbo);
  
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
    float layerAlpha = fboParameters.getFloat(i);
    ++i;
    if (layerAlpha == 0.0f) return;
    
    float finalAlpha = layerAlpha * dlptr->pauseAlpha;
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

void Synth::updateCompositeSideImages() {
  if (sidePanelWidth <= 0.0) return;
  
  float leftCycleElapsed = (ofGetElapsedTimef() - leftSidePanelLastUpdate) / leftSidePanelTimeoutSecs;
  float rightCycleElapsed = (ofGetElapsedTimef() - rightSidePanelLastUpdate) / rightSidePanelTimeoutSecs;

  // old panels fade out; new panels fade in
  ofEnableBlendMode(OF_BLENDMODE_ALPHA);

  // Easing helper
  auto easeIn = [](float x){ return x * x * x; };

  ofFloatColor backgroundColor = backgroundColorController.value;
  backgroundColor *= backgroundMultiplierParameter; backgroundColor.a = 1.0;

  leftPanelCompositeFbo.begin();
  {
    ofClear(backgroundColor);
    float alphaIn = easeIn(leftCycleElapsed); // drop quickly, then ease
    
    float alphaOut = 1.0f - alphaIn;
    if (hibernationState != HibernationState::ACTIVE) {
      alphaOut *= hibernationAlpha;
      alphaIn *= hibernationAlpha;
    }
    
    ofSetColor(ofFloatColor { 1.0, 1.0, 1.0, alphaOut });
    leftPanelFbo.getTarget().draw(0.0, 0.0);
    ofSetColor(ofFloatColor { 1.0, 1.0, 1.0, alphaIn });
    leftPanelFbo.getSource().draw(0.0, 0.0);
  }
  leftPanelCompositeFbo.end();

  rightPanelCompositeFbo.begin();
  {
    ofClear(backgroundColor);
    float alphaIn = easeIn(rightCycleElapsed);
    
    float alphaOut = 1.0f - alphaIn;
    if (hibernationState != HibernationState::ACTIVE) {
      alphaOut *= hibernationAlpha;
      alphaIn *= hibernationAlpha;
    }
    
    ofSetColor(ofFloatColor { 1.0, 1.0, 1.0, alphaOut });
    rightPanelFbo.getTarget().draw(0.0, 0.0);
    ofSetColor(ofFloatColor { 1.0, 1.0, 1.0, alphaIn });
    rightPanelFbo.getSource().draw(0.0, 0.0);
  }
  rightPanelCompositeFbo.end();
}

void Synth::drawSidePanels(float xleft, float xright, float w, float h) {
  ofSetColor(ofFloatColor { 1.0f, 1.0f, 1.0f, 1.0f });
  tonemapShader.begin(toneMapTypeParameter, sideExposureParameter, gammaParameter, whitePointParameter, contrastParameter, saturationParameter, brightnessParameter, hueShiftParameter);
  leftPanelCompositeFbo.draw(xleft, 0.0, w, h);
  rightPanelCompositeFbo.draw(xright, 0.0, w, h);
  tonemapShader.end();
}

void Synth::drawMiddlePanel(float w, float h, float scale) {
  ofPushMatrix();
  ofTranslate((w - imageCompositeFbo.getWidth() * scale) / 2.0, (h - imageCompositeFbo.getHeight() * scale) / 2.0);
  ofScale(scale, scale);
  
  if (transitionState == TransitionState::CROSSFADING && transitionSnapshotFbo.isAllocated() && transitionBlendFbo.isAllocated()) {
    // Blend snapshot and live composite into blend FBO
    transitionBlendFbo.begin();
    {
      ofClear(0, 0, 0, 255);
      ofEnableBlendMode(OF_BLENDMODE_ALPHA);
      
      // Draw old snapshot (fading out)
      ofSetColor(ofFloatColor { 1.0f, 1.0f, 1.0f, 1.0f - transitionAlpha });
      transitionSnapshotFbo.draw(0.0, 0.0);
      
      // Draw live composite (fading in)
      ofSetColor(ofFloatColor { 1.0f, 1.0f, 1.0f, transitionAlpha });
      imageCompositeFbo.draw(0.0, 0.0);
      
      ofEnableBlendMode(OF_BLENDMODE_DISABLED);
    }
    transitionBlendFbo.end();
    
    // Draw blended result to screen with tonemap
    tonemapShader.begin(toneMapTypeParameter, exposureParameter, gammaParameter, whitePointParameter, contrastParameter, saturationParameter, brightnessParameter, hueShiftParameter);
    ofSetColor(255);
    transitionBlendFbo.draw(0.0, 0.0);
    tonemapShader.end();
  } else {
    tonemapShader.begin(toneMapTypeParameter, exposureParameter, gammaParameter, whitePointParameter, contrastParameter, saturationParameter, brightnessParameter, hueShiftParameter);
    ofSetColor(255);
    imageCompositeFbo.draw(0.0, 0.0);
    tonemapShader.end();
  }
  
  ofPopMatrix();
}

void Synth::drawDebugViews() {
  ofPushMatrix();
  {
    ofTranslate((ofGetWindowWidth() - imageCompositeFbo.getWidth() * compositeScale) / 2.0, (ofGetWindowHeight() - imageCompositeFbo.getHeight() * compositeScale) / 2.0);
    // For Mods that draw directly and not on an FBO,
    // for example audio data plots and other debug views
    ofScale(ofGetWindowHeight(), ofGetWindowHeight()); // everything needs to draw in [0,1]x[0,1]
    std::for_each(modPtrs.cbegin(), modPtrs.cend(), [](const auto& pair) {
      const auto& [name, modPtr] = pair;
      modPtr->draw();
    });
  }
  ofPopMatrix();
}

// Does not draw the GUI: see drawGui()
void Synth::draw() {
  TSGL_START("Synth::draw");
  ofBlendMode(OF_BLENDMODE_DISABLED);
  drawSidePanels(0.0, ofGetWindowWidth() - sidePanelWidth, sidePanelWidth, sidePanelHeight);
  drawMiddlePanel(ofGetWindowWidth(), ofGetWindowHeight(), compositeScale);
  drawDebugViews();
  TSGL_STOP("Synth::draw");

#ifdef TARGET_MAC
  // Maybe this should be in update()?
  if (!paused && recorder.isRecording()) {
    recorderCompositeFbo.begin();
    float scale = recorderCompositeFbo.getHeight() / imageCompositeFbo.getHeight(); // could precompute this
    float sidePanelWidth = (recorderCompositeFbo.getWidth() - imageCompositeFbo.getWidth() * scale) / 2.0; // could precompute this
    drawSidePanels(0.0, recorderCompositeFbo.getWidth() - sidePanelWidth, sidePanelWidth, sidePanelHeight);
    drawMiddlePanel(recorderCompositeFbo.getWidth(), recorderCompositeFbo.getHeight(), scale);
    recorderCompositeFbo.end();
    
    TS_START("Synth::draw readToPixels");
    
    int width = recorderCompositeFbo.getWidth();
    int height = recorderCompositeFbo.getHeight();
    
    // Bind FBO for reading
    recorderCompositeFbo.bind();
    
    // Start async read into current PBO (non-blocking)
    recorderPbos[recorderPboWriteIndex].bind(GL_PIXEL_PACK_BUFFER);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, 0);
    recorderPbos[recorderPboWriteIndex].unbind(GL_PIXEL_PACK_BUFFER);
    
    recorderCompositeFbo.unbind();
    
    // Read from previous PBO (should be ready now) - but only after first frame
    if (recorderFrameCount > 0) {
      int readIndex = (recorderPboWriteIndex + 1) % NUM_PBOS;
      recorderPbos[readIndex].bind(GL_PIXEL_PACK_BUFFER);
      void* ptr = recorderPbos[readIndex].map(GL_READ_ONLY);
      if (ptr) {
        memcpy(recorderPixels.getData(), ptr, width * height * 3);
        recorderPbos[readIndex].unmap();
      }
      recorderPbos[readIndex].unbind(GL_PIXEL_PACK_BUFFER);
      recorder.addFrame(recorderPixels);
    }
    
    recorderPboWriteIndex = (recorderPboWriteIndex + 1) % NUM_PBOS;
    recorderFrameCount++;
    
    TS_STOP("Synth::draw readToPixels");
  }
#endif
  
  processPendingImageSave();
  initiateImageSaveTransfer();  // Issue glReadPixels here, after all rendering complete
}

void Synth::setAudioDataSourceMod(std::weak_ptr<AudioDataSourceMod> mod) {
  audioDataSourceModPtr = mod;
}

void Synth::drawGui() {
  if (!guiVisible) return;
  gui.draw();
}

void Synth::toggleRecording() {
#ifdef TARGET_MAC
  if (recorder.isRecording()) {
    // Flush the last pending frame from PBO before stopping
    if (recorderFrameCount > 0) {
      int readIndex = (recorderPboWriteIndex + NUM_PBOS - 1) % NUM_PBOS;
      recorderPbos[readIndex].bind(GL_PIXEL_PACK_BUFFER);
      void* ptr = recorderPbos[readIndex].map(GL_READ_ONLY);
      if (ptr) {
        memcpy(recorderPixels.getData(), ptr, recorderCompositeSize.x * recorderCompositeSize.y * 3);
        recorderPbos[readIndex].unmap();
        recorder.addFrame(recorderPixels);
      }
      recorderPbos[readIndex].unbind(GL_PIXEL_PACK_BUFFER);
    }
    
    recorder.stop();
    
    // Stop audio segment recording
    if (auto audioMod = audioDataSourceModPtr.lock()) {
      audioMod->stopSegmentRecording();
    }
    
  } else {
    std::string timestamp = ofGetTimestampString();
    recorder.setOutputPath(Synth::saveArtefactFilePath(VIDEOS_FOLDER_NAME+"/"+name+"/drawing-"+timestamp+".mp4"));
    // Start audio segment recording with matching timestamp
    if (auto audioMod = audioDataSourceModPtr.lock()) {
      audioMod->startSegmentRecording(
        Synth::saveArtefactFilePath(VIDEOS_FOLDER_NAME+"/"+name+"/audio-"+timestamp+".wav"));
    }
    
    // Reset PBO state for new recording
    recorderPboWriteIndex = 0;
    recorderFrameCount = 0;
    
    recorder.startCustomRecord();
  }
#endif
}

void Synth::saveImage() {
  // Silently ignore if a save is already in progress or requested
  if (pendingImageSave || imageSaveRequested) return;
  
  // Just set flag - actual GL work happens at end of draw() to avoid mid-frame stalls
  imageSaveRequested = true;
}

void Synth::initiateImageSaveTransfer() {
  // Called at end of draw(), after all rendering is complete
  if (!imageSaveRequested) return;
  imageSaveRequested = false;
  
  // Capture timestamp (system call, but we're past the critical rendering path)
  std::string timestamp = ofGetTimestampString();
  
  int w = imageCompositeFbo.getWidth();
  int h = imageCompositeFbo.getHeight();
  
  // Bind FBO only to GL_READ_FRAMEBUFFER to avoid affecting draw state
  glBindFramebuffer(GL_READ_FRAMEBUFFER, imageCompositeFbo.getId());
  glBindBuffer(GL_PIXEL_PACK_BUFFER, imageSavePbo.getId());
  glReadPixels(0, 0, w, h, GL_RGB, GL_FLOAT, 0);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  
  // Create fence to track completion
  GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  
  // Store pending state
  PendingImageSave pending;
  pending.timestamp = std::move(timestamp);
  pending.fence = fence;
  pendingImageSave = std::move(pending);
  
  saveStatus = "Fetching";
}

void Synth::processPendingImageSave() {
  // Clear saveStatus after timeout
  if (saveStatusClearTime > 0.0f && ofGetElapsedTimef() > saveStatusClearTime) {
    saveStatus = "";
    saveStatusClearTime = 0.0f;
  }
  
  if (!pendingImageSave) return;
  
  pendingImageSave->framesSinceRequested++;
  
  // Non-blocking check if fence is signaled
  GLenum result = glClientWaitSync(pendingImageSave->fence, 0, 0);
  
  if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED) {
    // Transfer complete - generate filepath now (deferred from saveImage to avoid stutter)
    std::string filepath = Synth::saveArtefactFilePath(
      SNAPSHOTS_FOLDER_NAME+"/"+name+"/drawing-"+pendingImageSave->timestamp+".exr");
    ofLogNotice("Synth") << "Saving drawing to " << filepath;
    
    // Map PBO and copy to pixels
    int w = imageCompositeFbo.getWidth();
    int h = imageCompositeFbo.getHeight();
    size_t size = static_cast<size_t>(w) * static_cast<size_t>(h) * 3 * sizeof(float);
    
    ofFloatPixels pixels;
    pixels.allocate(w, h, OF_IMAGE_COLOR);
    
    imageSavePbo.bind(GL_PIXEL_PACK_BUFFER);
    void* ptr = imageSavePbo.map(GL_READ_ONLY);
    if (ptr) {
      memcpy(pixels.getData(), ptr, size);
      imageSavePbo.unmap();
    }
    imageSavePbo.unbind(GL_PIXEL_PACK_BUFFER);
    
    // Clean up fence
    glDeleteSync(pendingImageSave->fence);
    
    // Hand off to save thread
    pruneSaveThreads();
    auto threadPtr = std::make_unique<SaveToFileThread>();
    threadPtr->save(filepath, std::move(pixels));
    saveToFileThreads.push_back(std::move(threadPtr));
    
    // Extract filename for status
    std::string filename = filepath.substr(filepath.find_last_of("/") + 1);
    saveStatus = "Saving: " + filename;
    saveStatusClearTime = ofGetElapsedTimef() + 5.0f;  // Clear after 5 seconds
    
    pendingImageSave.reset();
    
  } else if (result == GL_WAIT_FAILED || pendingImageSave->framesSinceRequested > 300) {
    // Timeout or error - abandon save
    ofLogError("Synth") << "Failed to fetch pixels for image save (frames waited: " 
                        << pendingImageSave->framesSinceRequested << ")";
    glDeleteSync(pendingImageSave->fence);
    saveStatus = "Save failed";
    saveStatusClearTime = ofGetElapsedTimef() + 5.0f;
    pendingImageSave.reset();
  }
  // else GL_TIMEOUT_EXPIRED - leave for next frame
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
    paused = not paused;
    // We don't need to pause Mods individually yet, but if we did:
//    std::for_each(modPtrs.cbegin(), modPtrs.cend(), [this](auto& modPtr) {
//      modPtr->setPaused(paused);
//    });
    return true;
  }
  
  if (key == 'H') {
    if (hibernationState == HibernationState::ACTIVE) {
      startHibernation();
    } else {
      cancelHibernation();
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
  
  // Allocate blend FBO if needed
  if (!transitionBlendFbo.isAllocated() || 
      transitionBlendFbo.getWidth() != w ||
      transitionBlendFbo.getHeight() != h) {
    transitionBlendFbo.allocate(w, h, GL_RGB16F);
  }
  
  transitionSnapshotFbo.begin();
  ofSetColor(255);
  imageCompositeFbo.draw(0, 0);
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
  fboParameters.clear();
  fboParamPtrs.clear();
  
  fboParameters.setName("Layers");
  std::for_each(drawingLayerPtrs.cbegin(), drawingLayerPtrs.cend(), [this](const auto& pair) {
    const auto& [name, fcptr] = pair;
    if (!fcptr->isDrawn) return;
    float initialAlpha = 1.0f;
    auto it = initialLayerAlphas.find(fcptr->name);
    if (it != initialLayerAlphas.end()) {
      initialAlpha = it->second;
    }
    auto fboParam = std::make_shared<ofParameter<float>>(fcptr->name, initialAlpha, 0.0f, 1.0f);
    fboParamPtrs.push_back(fboParam);
    fboParameters.add(*fboParam);
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
    layerPtr->pauseAlpha = initialPaused ? 0.0f : 1.0f;
    layerPtr->pauseAlphaAtFadeStart = layerPtr->pauseAlpha;
  });
}

void Synth::initIntentParameterGroup() {
  intentParameters.clear();
  
  intentParameters.setName("Intent");
  intentParameters.add(intentStrengthParameter);
  // Don't recreate intents here - they should be set via setIntentPresets() or initIntentPresets()
  // Just add the existing activation parameters to the group
  for (auto& p : intentActivationParameters) intentParameters.add(*p);
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
  float now = ofGetElapsedTimef();
  float duration = std::max(0.001f, layerPauseFadeDurationSecParameter.get());
  
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
    
    auto& state = layerPtr->pauseState;
    float& alpha = layerPtr->pauseAlpha;
    
    // Start fades based on target
    if (targetPaused) {
      if (state == DrawingLayer::PauseState::ACTIVE ||
          state == DrawingLayer::PauseState::FADING_IN) {
        state = DrawingLayer::PauseState::FADING_OUT;
        layerPtr->pauseFadeStartTime = now;
        layerPtr->pauseAlphaAtFadeStart = alpha;
      }
    } else {
      if (state == DrawingLayer::PauseState::PAUSED ||
          state == DrawingLayer::PauseState::FADING_OUT) {
        state = DrawingLayer::PauseState::FADING_IN;
        layerPtr->pauseFadeStartTime = now;
        layerPtr->pauseAlphaAtFadeStart = alpha;
      }
    }
    
    // Advance fades
    switch (state) {
      case DrawingLayer::PauseState::ACTIVE:
        alpha = 1.0f;
        break;
      case DrawingLayer::PauseState::PAUSED:
        alpha = 0.0f;
        break;
      case DrawingLayer::PauseState::FADING_OUT: {
        float t = ofClamp((now - layerPtr->pauseFadeStartTime) / duration, 0.0f, 1.0f);
        alpha = ofLerp(layerPtr->pauseAlphaAtFadeStart, 0.0f, t);
        if (t >= 1.0f) {
          state = DrawingLayer::PauseState::PAUSED;
          alpha = 0.0f;
        }
        break;
      }
      case DrawingLayer::PauseState::FADING_IN: {
        float t = ofClamp((now - layerPtr->pauseFadeStartTime) / duration, 0.0f, 1.0f);
        alpha = ofLerp(layerPtr->pauseAlphaAtFadeStart, 1.0f, t);
        if (t >= 1.0f) {
          state = DrawingLayer::PauseState::ACTIVE;
          alpha = 1.0f;
        }
        break;
      }
    }
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
  parameters.add(layerPauseFadeDurationSecParameter);
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
  } else {
    ofLogError("Synth") << "Failed to load config from: " << filepath;
  }
  
  return success;
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
  loadFromConfig(filepath);
  initParameters();
  initFboParameterGroup();
  initIntentParameterGroup();
  gui.markNodeEditorDirty();
  
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



} // ofxMarkSynth
