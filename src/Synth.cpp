//
//  Synth.cpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#include "Synth.hpp"
#include "ofxTimeMeasurements.h"
#include "ofConstants.h"
#include "GuiUtil.hpp"



namespace ofxMarkSynth {



std::string saveFilePath(const std::string& filename) {
  return ofFilePath::getUserHomeDir()+"/Documents/MarkSynth/"+filename;
}

constexpr std::string SETTINGS_FOLDER_NAME = "settings";
constexpr std::string SNAPSHOTS_FOLDER_NAME = "drawings";
constexpr std::string VIDEOS_FOLDER_NAME = "drawing-recordings";



Synth::Synth(const std::string& name_, const ModConfig&& config, bool startPaused, glm::vec2 compositeSize_) :
Mod(name_, std::move(config)),
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
}

// FIXME: this has to be called after all Mods are added to build the GUI from the child GUIs
void Synth::configureGui() {
  parameters = getParameterGroup();
  gui.setup(parameters);
  minimizeAllGuiGroupsRecursive(gui);

  gui.add(pauseStatus.setup("Paused", ""));
  gui.add(recorderStatus.setup("Recording", ""));
  gui.add(saveStatus.setup("# Image Saves", ""));
}

void Synth::shutdown() {
  ofLogNotice() << "Synth::shutdown " << name << std::endl;
  
  std::for_each(mods.cbegin(), mods.cend(), [](const auto& pair) {
    const auto& [name, modPtr] = pair;
    modPtr->shutdown();
  });
  
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

FboPtr Synth::addFboConfig(std::string name, glm::vec2 size, GLint internalFormat, int wrap, bool clearOnUpdate, ofBlendMode blendMode, bool useStencil, int numSamples) {
  auto fboPtr = std::make_shared<PingPongFbo>();
  fboPtr->allocate(size, internalFormat, wrap, useStencil, numSamples);
  fboPtr->clearFloat(DEFAULT_CLEAR_COLOR);
  FboConfigPtr fboConfigPtr = std::make_shared<FboConfig>(name, fboPtr, clearOnUpdate, blendMode);
  fboConfigPtrs.insert({ name, fboConfigPtr });
  return fboPtr;
}

void Synth::receive(int sinkId, const glm::vec4& v) {
  switch (sinkId) {
    case SINK_BACKGROUND_COLOR:
      backgroundColorParameter = ofFloatColor { v.r, v.g, v.b, v.a };
      break;
    default:
      ofLogError() << "glm::vec4 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void Synth::receive(int sinkId, const float& v) {
  switch (sinkId) {
    case SINK_AUDIO_ONSET:
    case SINK_AUDIO_TIMBRE_CHANGE:
      {
        // make a map of ModPtr to "bid" for this change then find the highest bid and action that Mod
        std::map<ModPtr, float> modBids;
        std::for_each(mods.cbegin(), mods.cend(), [sinkId, &modBids](const auto& pair) {
          const auto& [name, modPtr] = pair;
          modBids[modPtr] = modPtr->bidToReceive(sinkId);
        });
        auto winningModIt = std::max_element(modBids.cbegin(), modBids.cend(), [](const auto& a, const auto& b) {
          return a.second < b.second;
        });
        if (winningModIt != modBids.cend() && winningModIt->second > 0.0) {
          ofLogNotice() << "Synth " << name << " awarding " << (sinkId == SINK_AUDIO_ONSET ? "onset" : "timbre change") << " to Mod " << winningModIt->first->name << " with bid " << winningModIt->second;
          winningModIt->first->receive(sinkId, v);
        }
      }
      break;
    default:
      ofLogError() << "glm::vec4 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void Synth::update() {
  pauseStatus = paused ? "Yes" : "No";
  recorderStatus = recorder.isRecording() ? "Yes" : "No";
  saveStatus = ofToString(SaveToFileThread::activeThreadCount);
  
  if (paused) return;
  
  std::for_each(fboConfigPtrs.cbegin(), fboConfigPtrs.cend(), [this](const auto& pair) {
    const auto& [name, fcptr] = pair;
    if (fcptr->clearOnUpdate) {
      fcptr->fboPtr->getSource().begin();
      ofClear(DEFAULT_CLEAR_COLOR);
      fcptr->fboPtr->getSource().end();
    }
  });
  
  std::for_each(mods.cbegin(), mods.cend(), [](const auto& pair) {
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
    ofFloatColor backgroundColor = backgroundColorParameter;
    backgroundColor *= backgroundMultiplierParameter; backgroundColor.a = 1.0;
    ofClear(backgroundColor);
    
    size_t i = 0;
    std::for_each(fboConfigPtrs.cbegin(), fboConfigPtrs.cend(), [this, &i](const auto& pair) {
      const auto& [name, fcptr] = pair;
      ofEnableBlendMode(fcptr->blendMode);
      float layerAlpha = fboParameters.getFloat(i);
      ++i;
      if (layerAlpha == 0.0) return;
      ofSetColor(ofFloatColor { 1.0, 1.0, 1.0, layerAlpha });
      fcptr->fboPtr->draw(0, 0, imageCompositeFbo.getWidth(), imageCompositeFbo.getHeight());
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

  // Easing helpers (clamped to [0,1])
  auto easeIn = [&](float x){ return x * x * x; };

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
  ofSetColor(255);
  ofSetColor(ofFloatColor { 1.0, 0.0, 0.0, 0.3f });
  tonemapShader.begin(toneMapTypeParameter, sideExposureParameter, gammaParameter, whitePointParameter);
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
      tonemapShader.begin(toneMapTypeParameter, exposureParameter, gammaParameter, whitePointParameter);
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
    std::for_each(mods.cbegin(), mods.cend(), [](const auto& pair) {
      const auto& [name, modPtr] = pair;
      modPtr->draw();
    });
  }
  ofPopMatrix();
}

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
  if (guiVisible) gui.draw();
}

void Synth::setGuiSize(glm::vec2 size) {
  gui.setPosition(0.0, 0.0);
  gui.setSize(size.x, size.y);
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
      gui.saveToFile(saveFilePath(SETTINGS_FOLDER_NAME+"/"+name+"/settings-"+char(key)+".json"));
      plusKeyPressed = false;
      return true;
    } else if (equalsKeyPressed) {
      gui.loadFromFile(saveFilePath(SETTINGS_FOLDER_NAME+"/"+name+"/settings-"+char(key)+".json"));
      equalsKeyPressed = false;
      return true;
    }
  } else {
    equalsKeyPressed = false;
    plusKeyPressed = false;
  }
  // <<<
  
  if (key == 'S') {
    SaveToFileThread* threadPtr = new SaveToFileThread();
    std::string filepath = saveFilePath(SNAPSHOTS_FOLDER_NAME+"/"+name+"/drawing-"+ofGetTimestampString()+".exr");
    ofLogNotice() << "Fetch drawing to save to " << filepath;
    ofFloatPixels pixels;
    pixels.allocate(imageCompositeFbo.getWidth(), imageCompositeFbo.getHeight(), OF_IMAGE_COLOR);
    imageCompositeFbo.readToPixels(pixels);
    threadPtr->save(filepath, std::move(pixels));
    saveToFileThreads.push_back(threadPtr);
    return true;
  }

#ifdef TARGET_MAC
  if (key == 'R') {
    if (recorder.isRecording()) {
      recorder.stop();
      ofSetWindowTitle("");
    } else {
      recorder.setOutputPath(saveFilePath(VIDEOS_FOLDER_NAME+"/"+name+"/drawing-"+ofGetTimestampString()+".mp4"));
      recorder.startCustomRecord();
      ofSetWindowTitle("[Recording]");
    }
  }
#endif

  bool handled = std::any_of(mods.cbegin(), mods.cend(), [&key](const auto& pair) {
    const auto& [name, modPtr] = pair;
    return modPtr->keyPressed(key);
  });
  return handled;
}

ofParameterGroup& Synth::getFboParameterGroup() {
  if (fboParameters.size() == 0) {
    fboParameters.setName("Layers");
    std::for_each(fboConfigPtrs.cbegin(), fboConfigPtrs.cend(), [this](const auto& pair) {
      const auto& [name, fcptr] = pair;
      auto fboParam = std::make_shared<ofParameter<float>>(fcptr->name, 1.0, 0.0, 1.0);
      fboParamPtrs.push_back(fboParam);
      fboParameters.add(*fboParam);
    });
  }
  return fboParameters;
}

void Synth::initParameters() {
  displayParameters.setName("Display");
  displayParameters.add(backgroundColorParameter);
  displayParameters.add(backgroundMultiplierParameter);
  displayParameters.add(toneMapTypeParameter);
  displayParameters.add(exposureParameter);
  displayParameters.add(gammaParameter);
  displayParameters.add(whitePointParameter);
  displayParameters.add(sideExposureParameter);
  parameters.add(displayParameters);
  parameters.add(getFboParameterGroup());
  std::for_each(mods.cbegin(), mods.cend(), [this](const auto& pair) {
    const auto& [name, modPtr] = pair;
    ofParameterGroup& pg = modPtr->getParameterGroup();
    if (pg.size() != 0) parameters.add(pg);
  });
}



} // ofxMarkSynth
