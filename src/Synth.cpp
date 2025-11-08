//
//  Synth.cpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#include "Synth.hpp"
#include "ofxTimeMeasurements.h"
#include "ofConstants.h"
#include "Gui.hpp"



namespace ofxMarkSynth {



std::string saveFilePath(const std::string& filename) {
  return ofFilePath::getUserHomeDir()+"/Documents/MarkSynth/"+filename;
}

constexpr std::string SETTINGS_FOLDER_NAME = "settings";
constexpr std::string SNAPSHOTS_FOLDER_NAME = "drawings";
constexpr std::string VIDEOS_FOLDER_NAME = "drawing-recordings";



Synth::Synth(const std::string& name_, const ModConfig&& config, bool startPaused, glm::vec2 compositeSize_) :
Mod(nullptr, name_, std::move(config)),
paused { startPaused },
compositeSize { compositeSize_ }
{
  imageCompositeFbo.allocate(compositeSize.x, compositeSize.y, GL_RGB16F);
  compositeScale = std::min(ofGetWindowWidth() / imageCompositeFbo.getWidth(), ofGetWindowHeight() / imageCompositeFbo.getHeight());
  
  sidePanelWidth = (ofGetWindowWidth() - imageCompositeFbo.getWidth() * compositeScale) / 2.0 - 8.0;
  if (sidePanelWidth > 0.0) {
    sidePanelHeight = ofGetWindowHeight();
    leftPanelFbo.allocate(sidePanelWidth, sidePanelHeight, GL_RGB16F);
    leftPanelCompositeFbo.allocate(sidePanelWidth, sidePanelHeight, GL_RGB16F);
    rightPanelFbo.allocate(sidePanelWidth, sidePanelHeight, GL_RGB16F);
    rightPanelCompositeFbo.allocate(sidePanelWidth, sidePanelHeight, GL_RGB16F);
  }

  tonemapShader.load();
  
#ifdef TARGET_MAC
  std::filesystem::create_directories(saveFilePath(SETTINGS_FOLDER_NAME+"/"+name));
  std::filesystem::create_directories(saveFilePath(SNAPSHOTS_FOLDER_NAME+"/"+name));
  std::filesystem::create_directories(saveFilePath(VIDEOS_FOLDER_NAME+"/"+name));
#endif

#ifdef TARGET_MAC
  recorderCompositeFbo.allocate(1920, 1080, GL_RGB);
  recorder.setup(/*video*/true, /*audio*/false, recorderCompositeFbo.getSize(), /*fps*/30.0, /*bitrate*/12000);
  recorder.setOverWrite(true);
  recorder.setFFmpegPathToAddonsPath();
  recorder.setInputPixelFormat(OF_IMAGE_COLOR);
//  recorder.setVideoCodec("libx264"); // doesn't work, nor h265
#endif
  
  of::random::seed(0);
  
  sourceNameIdMap = {
    { "compositeFbo", SOURCE_COMPOSITE_FBO }
  };
  sinkNameIdMap = {
    { backgroundColorParameter.getName(), SINK_BACKGROUND_COLOR },
    { "resetRandomness", SINK_RESET_RANDOMNESS }
  };
}

void Synth::configureGui(std::shared_ptr<ofAppBaseWindow> windowPtr) {
  initDisplayParameterGroup();
  initFboParameterGroup();
  initIntentParameterGroup();
  
  parameters = getParameterGroup();
  
  gui.setup(dynamic_pointer_cast<Synth>(shared_from_this()), windowPtr);
}

void Synth::shutdown() {
  ofLogNotice() << "Synth::shutdown " << name << std::endl;
  
  std::for_each(modPtrs.cbegin(), modPtrs.cend(), [](const auto& pair) {
    const auto& [name, modPtr] = pair;
    modPtr->shutdown();
  });
  
  gui.exit();
  
#ifdef TARGET_MAC
  if (recorder.isRecording()) {
    ofLogNotice() << "Stopping recording" << std::endl;
    recorder.stop();
    ofLogNotice() << "Recording stopped" << std::endl;
  }
#endif
  
  std::for_each(saveToFileThreads.begin(), saveToFileThreads.end(), [](auto& thread) {
    ofLogNotice() << "Waiting for save thread to finish" << std::endl;
    thread->waitForThread(false);
    ofLogNotice() << "Done waiting for save thread to finish" << std::endl;
  });
}

DrawingLayerPtr Synth::addDrawingLayer(std::string name, glm::vec2 size, GLint internalFormat, int wrap, float fadeBy, ofBlendMode blendMode, bool useStencil, int numSamples, bool isDrawn) {
  auto fboPtr = std::make_shared<PingPongFbo>();
  fboPtr->allocate(size, internalFormat, wrap, useStencil, numSamples);
  fboPtr->clearFloat(DEFAULT_CLEAR_COLOR);
  DrawingLayerPtr drawingLayerPtr = std::make_shared<DrawingLayer>(name, fboPtr, fadeBy, blendMode, isDrawn);
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
      ofLogError() << "Synth::addConnections: Unknown source mod name: " << sourceModName;
      continue;
    }
    auto sourceModPtr = sourceModName.empty() ? shared_from_this() : modPtrs.at(sourceModName);
    
    if(!sinkModName.empty() && !modPtrs.contains(sinkModName)) {
      ofLogError() << "Synth::addConnections: Unknown sink mod name: " << sinkModName;
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
      ofLogError() << "glm::vec4 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void Synth::receive(int sinkId, const float& v) {
  switch (sinkId) {
    case SINK_RESET_RANDOMNESS:
      {
        // Use bucketed onset value as seed for repeatability
        int seed = static_cast<int>(v * 10.0f); // Adjust multiplier for desired granularity
        of::random::seed(seed);
        ofLogNotice() << "Reset seed: " << seed;
      }
      break;
    default:
      ofLogError() << "float receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void Synth::applyIntent(const Intent& intent, float intentStrength) {
  // Structure & Inverse Chaos -> background brightness
  float structure = intent.getStructure();
  float chaos = intent.getChaos();
  float brightness = ofLerp(0.0f, 0.15f, structure) * (1.0f - chaos * 0.5f);
  ofFloatColor target = ofFloatColor(brightness, brightness, brightness, 1.0f);
  backgroundColorController.updateIntent(target, intentStrength);
}

void Synth::update() {
  pauseStatus = paused ? "Yes" : "No";
  recorderStatus = recorder.isRecording() ? "Yes" : "No";
  saveStatus = ofToString(SaveToFileThread::activeThreadCount);
  
  if (paused) return;
  
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
  
  std::for_each(drawingLayerPtrs.cbegin(), drawingLayerPtrs.cend(), [this](const auto& pair) {
    const auto& [name, fcptr] = pair;
    if (fcptr->clearOnUpdate) {
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
  
  TSGL_START("Synth-updateComposites");
  TS_START("Synth-updateComposites");
  updateCompositeImage();
  updateCompositeSideImages();
  updateSidePanels();
  TS_STOP("Synth-updateComposites");
  TSGL_STOP("Synth-updateComposites");
  
  emit(Synth::SOURCE_COMPOSITE_FBO, imageCompositeFbo);
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
  imageCompositeFbo.begin();
  {
    ofFloatColor backgroundColor = backgroundColorController.value;
    backgroundColor *= backgroundMultiplierParameter; backgroundColor.a = 1.0;
    ofClear(backgroundColor);
    
    size_t i = 0;
    std::for_each(drawingLayerPtrs.cbegin(), drawingLayerPtrs.cend(), [this, &i](const auto& pair) {
      const auto& [name, dlptr] = pair;
      if (!dlptr->isDrawn) return;
      ofEnableBlendMode(dlptr->blendMode);
      float layerAlpha = fboParameters.getFloat(i);
      ++i;
      if (layerAlpha == 0.0) return;
      ofSetColor(ofFloatColor { 1.0, 1.0, 1.0, layerAlpha });
      dlptr->fboPtr->draw(0, 0, imageCompositeFbo.getWidth(), imageCompositeFbo.getHeight());
    });
  }
  imageCompositeFbo.end();
}

void Synth::updateCompositeSideImages() {
  if (sidePanelWidth <= 0.0) return;
  
  float leftCycleElapsed = (ofGetElapsedTimef() - leftSidePanelLastUpdate) / leftSidePanelTimeoutSecs;
  float rightCycleElapsed = (ofGetElapsedTimef() - rightSidePanelLastUpdate) / rightSidePanelTimeoutSecs;

  // old panels fade out; new panels fade in
  ofEnableBlendMode(OF_BLENDMODE_ALPHA);

  // Easing helper
  auto easeIn = [](float x){ return x * x * x; };

  // Left panel
  leftPanelCompositeFbo.begin();
  {
    float alphaIn = easeIn(leftCycleElapsed); // drop quickly, then ease
    ofSetColor(ofFloatColor { 1.0, 1.0, 1.0, 1.0f - alphaIn });
    leftPanelFbo.getTarget().draw(0.0, 0.0);
    ofSetColor(ofFloatColor { 1.0, 1.0, 1.0, alphaIn });
    leftPanelFbo.getSource().draw(0.0, 0.0);
  }
  leftPanelCompositeFbo.end();

  // Right panel
  rightPanelCompositeFbo.begin();
  {
    float alphaIn = easeIn(rightCycleElapsed);
    ofSetColor(ofFloatColor { 1.0, 1.0, 1.0, 1.0f - alphaIn });
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
  {
    ofTranslate((w - imageCompositeFbo.getWidth() * scale) / 2.0, (h - imageCompositeFbo.getHeight() * scale) / 2.0);
    
    ofPushMatrix();
    {
      ofScale(scale, scale);
      ofSetColor(255);
      tonemapShader.begin(toneMapTypeParameter, exposureParameter, gammaParameter, whitePointParameter, contrastParameter, saturationParameter, brightnessParameter, hueShiftParameter);
      imageCompositeFbo.draw(0.0, 0.0);
      tonemapShader.end();
    }
    ofPopMatrix();
    
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

#ifdef TARGET_MAC
  if (!paused && recorder.isRecording()) {
    recorderCompositeFbo.begin();
    float scale = recorderCompositeFbo.getHeight() / imageCompositeFbo.getHeight(); // could precompute this
    float sidePanelWidth = (recorderCompositeFbo.getWidth() - imageCompositeFbo.getWidth() * scale) / 2.0; // could precompute this
    drawSidePanels(0.0, recorderCompositeFbo.getWidth() - sidePanelWidth, sidePanelWidth, sidePanelHeight);
    drawMiddlePanel(recorderCompositeFbo.getWidth(), recorderCompositeFbo.getHeight(), scale);
    recorderCompositeFbo.end();
    ofPixels pixels;
    recorderCompositeFbo.readToPixels(pixels);
    recorder.addFrame(pixels);
  }
#endif  
  TSGL_STOP("Synth::draw");
}

void Synth::audioCallback(float* buffer, int bufferSize, int nChannels) {
  if (!recorder.isRecording()) return;
//  recorder.addBuffer(buffer, bufferSize, nChannels);
}

void Synth::drawGui() {
  if (!guiVisible) return;
  gui.draw();
}

void Synth::toggleRecording() {
#ifdef TARGET_MAC
  if (recorder.isRecording()) {
    recorder.stop();
    ofSetWindowTitle("");
  } else {
    recorder.setOutputPath(saveFilePath(VIDEOS_FOLDER_NAME+"/"+name+"/drawing-"+ofGetTimestampString()+".mp4"));
    recorder.startCustomRecord();
    ofSetWindowTitle("[Recording]");
  }
#endif
}

void Synth::saveImage() {
  SaveToFileThread* threadPtr = new SaveToFileThread();
  std::string filepath = saveFilePath(SNAPSHOTS_FOLDER_NAME+"/"+name+"/drawing-"+ofGetTimestampString()+".exr");
  ofLogNotice() << "Fetch drawing to save to " << filepath;
  ofFloatPixels pixels;
  pixels.allocate(imageCompositeFbo.getWidth(), imageCompositeFbo.getHeight(), OF_IMAGE_COLOR);
  imageCompositeFbo.readToPixels(pixels);
  threadPtr->save(filepath, std::move(pixels));
  saveToFileThreads.push_back(threadPtr);
}

bool Synth::keyPressed(int key) {
  if (key == OF_KEY_TAB) { guiVisible = not guiVisible; return true; }
  
  if (key == OF_KEY_SPACE) {
    paused = not paused;
    // We don't need to pause Mods individually yet, but if we did:
//    std::for_each(modPtrs.cbegin(), modPtrs.cend(), [this](auto& modPtr) {
//      modPtr->setPaused(paused);
//    });
    return true;
  }

  // >>> Deal with the `[+=][0-9]` chords
  if (key == '+') {
    plusKeyPressed = true;
    equalsKeyPressed = false;
    return true;
  }
  if (key == '=') {
    equalsKeyPressed = true;
    plusKeyPressed = false;
    return true;
  }
  if (key >= '0' && key <= '9') {
    if (plusKeyPressed) {
//      gui.saveToFile(saveFilePath(SETTINGS_FOLDER_NAME+"/"+name+"/settings-"+char(key)+".json"));
      plusKeyPressed = false;
      return true;
    } else if (equalsKeyPressed) {
//      gui.loadFromFile(saveFilePath(SETTINGS_FOLDER_NAME+"/"+name+"/settings-"+char(key)+".json"));
      equalsKeyPressed = false;
      return true;
    }
  } else {
    equalsKeyPressed = false;
    plusKeyPressed = false;
  }
  // <<<
  
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

void Synth::initFboParameterGroup() {
  fboParameters.setName("Layers");
  std::for_each(drawingLayerPtrs.cbegin(), drawingLayerPtrs.cend(), [this](const auto& pair) {
    const auto& [name, fcptr] = pair;
    if (!fcptr->isDrawn) return;
    auto fboParam = std::make_shared<ofParameter<float>>(fcptr->name, 1.0, 0.0, 1.0);
    fboParamPtrs.push_back(fboParam);
    fboParameters.add(*fboParam);
  });
}

void Synth::initIntentParameterGroup() {
  intentParameters.setName("Intent");
  intentParameters.add(intentStrengthParameter);
  initIntentPresets();
  for (auto& p : intentActivationParameters) intentParameters.add(*p);
}

void Synth::initDisplayParameterGroup() {
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

void Synth::initParameters() {
  parameters.add(agencyParameter);
  parameters.add(backgroundColorParameter);
  parameters.add(backgroundMultiplierParameter);
  std::for_each(modPtrs.cbegin(), modPtrs.cend(), [this](const auto& pair) {
    const auto& [name, modPtr] = pair;
    ofParameterGroup& pg = modPtr->getParameterGroup();
    if (pg.size() != 0) parameters.add(pg);
  });
}

void Synth::initIntentPresets() {
  // Remember fader 0 is a master control, and there are another 7 faders available
  std::vector<IntentPtr> presets = {
    Intent::createPreset("Calm", 0.2f, 0.3f, 0.7f, 0.1f, 0.1f),
    Intent::createPreset("Energetic", 0.9f, 0.7f, 0.4f, 0.5f, 0.5f),
    Intent::createPreset("Chaotic", 0.6f, 0.2f, 0.1f, 0.95f, 0.4f),
    Intent::createPreset("Dense", 0.5f, 0.95f, 0.6f, 0.3f, 0.5f),
    Intent::createPreset("Structured", 0.4f, 0.5f, 0.95f, 0.2f, 0.4f),
    Intent::createPreset("Minimal", 0.1f, 0.1f, 0.8f, 0.05f, 0.1f),
    Intent::createPreset("Maximum", 0.95f, 0.95f, 0.5f, 0.8f, 0.95f),
  };
  intentActivations.clear();
  intentActivationParameters.clear();
  for (auto& ip : presets) {
    intentActivations.emplace_back(ip);
    auto p = std::make_shared<ofParameter<float>>(ip->getName() + " Activation", 0.0f, 0.0f, 1.0f);
    intentActivationParameters.push_back(p);
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
  // Apply to Synth first, then to Mods
  applyIntent(activeIntent, intentStrengthParameter);
  for (auto& kv : modPtrs) {
    kv.second->applyIntent(activeIntent, intentStrengthParameter);
  }
}



} // ofxMarkSynth
