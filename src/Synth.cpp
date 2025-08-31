//
//  Synth.cpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#include "Synth.hpp"
#include "ofxTimeMeasurements.h"
#include "ofxTinyEXR.h"

namespace ofxMarkSynth {



void minimizeAllGuiGroupsRecursive(ofxGuiGroup& guiGroup) {
  for (int i = 0; i < guiGroup.getNumControls(); ++i) {
    auto control = guiGroup.getControl(i);
    if (auto childGuiGroup = dynamic_cast<ofxGuiGroup*>(control)) {
      childGuiGroup->minimize();
      minimizeAllGuiGroupsRecursive(*childGuiGroup);
    }
  }
}



void SaveToFileThread::save(const std::string& filepath_, ofFloatPixels&& pixels_)
{
  pixels = pixels_;
  filepath = filepath_;
  startThread();
}

void SaveToFileThread::threadedFunction() {
  ofLogNotice() << "Saving drawing to " << filepath;
  
  ofxTinyEXR exrIO;
  bool saved = exrIO.savePixels(pixels, filepath);
  if (!saved) ofLogWarning() << "Failed to save EXR image";
  
  ofLogNotice() << "Done saving drawing to " << filepath;
}



// See ofFbo.cpp #allocate
void allocateFbo(FboPtr fboPtr, glm::vec2 size, GLint internalFormat, int wrap, bool useStencil) {
  ofFboSettings settings { nullptr };
  settings.wrapModeVertical = wrap;
  settings.wrapModeHorizontal = wrap;
  
  settings.width = size.x;
  settings.height = size.y;
  settings.internalformat = internalFormat;
  settings.numSamples = 0;
  
  settings.useDepth = false;
  settings.useStencil = useStencil;
  settings.textureTarget = GL_TEXTURE_2D;

  fboPtr->allocate(settings);
}

void addFboConfigPtr(FboConfigPtrs& fboConfigPtrs, std::string name, FboPtr fboPtr, glm::vec2 size, GLint internalFormat, int wrap, ofFloatColor clearColor, bool clearOnUpdate, ofBlendMode blendMode, bool useStencil) {
  allocateFbo(fboPtr, size, internalFormat, wrap, useStencil);
  fboPtr->getSource().begin();
  fboPtr->getSource().clearColorBuffer(clearColor);
  fboPtr->getSource().end();
  fboConfigPtrs.emplace_back(std::make_shared<FboConfig>(name, fboPtr, clearColor, clearOnUpdate, blendMode));
}



std::string saveFilePath(const std::string& filename) {
  return ofFilePath::getUserHomeDir()+"/Documents/MarkSynth/"+filename;
}

constexpr std::string SETTINGS_FOLDER_NAME = "settings";
constexpr std::string SNAPSHOTS_FOLDER_NAME = "drawings";
constexpr std::string VIDEOS_FOLDER_NAME = "drawing-recordings";



Synth::Synth(const std::string& name_, const ModConfig&& config) :
Mod(name_, std::move(config))
{
  tonemapShader.load();
  
#ifndef TARGET_OS_IOS
  std::filesystem::create_directories(saveFilePath(SETTINGS_FOLDER_NAME+"/"+name));
  std::filesystem::create_directories(saveFilePath(SNAPSHOTS_FOLDER_NAME+"/"+name));
  std::filesystem::create_directories(saveFilePath(VIDEOS_FOLDER_NAME+"/"+name));
#endif

#ifndef TARGET_OS_IOS
  recorder.setup(/*video*/true, /*audio*/false, ofGetWindowSize(), /*fps*/30.0, /*bitrate*/10000);
  recorder.setOverWrite(true);
  recorder.setFFmpegPathToAddonsPath();
  recorder.setInputPixelFormat(OF_IMAGE_COLOR);
  recorderCompositeFbo.allocate(ofGetWindowWidth(), ofGetWindowHeight(), GL_RGB);
#endif
}

void Synth::shutdown() {
  ofLogNotice() << "Synth::shutdown " << name << std::endl;
  
#ifndef TARGET_OS_IOS
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

void Synth::configure(FboConfigPtrs&& fboConfigPtrs_, ModPtrs&& modPtrs_, glm::vec2 compositeSize_) {
  if (std::any_of(fboConfigPtrs.begin(), fboConfigPtrs.end(), [this](const auto& fcptr) {
    return fcptr->fboPtr->getSource().isAllocated();
  })) ofLogError() << "Unallocated FBO";
  fboConfigPtrs = std::move(fboConfigPtrs_);
  
  modPtrs = std::move(modPtrs_);
  
  imageCompositeFbo.allocate(compositeSize_.x, compositeSize_.y, GL_RGB16F);
  compositeScale = std::min(ofGetWindowWidth() / imageCompositeFbo.getWidth(), ofGetWindowHeight() / imageCompositeFbo.getHeight());
  sidePanelWidth = (ofGetWindowWidth() - imageCompositeFbo.getWidth() * compositeScale) / 2.0 - 8.0;
  sidePanelHeight = ofGetWindowHeight();
  leftPanelFbo.allocate(sidePanelWidth, compositeSize_.y, GL_RGB16F);
  leftPanelCompositeFbo.allocate(sidePanelWidth, compositeSize_.y, GL_RGB16F);
  rightPanelFbo.allocate(sidePanelWidth, compositeSize_.y, GL_RGB16F);
  rightPanelCompositeFbo.allocate(sidePanelWidth, compositeSize_.y, GL_RGB16F);

  parameters = getParameterGroup();
  gui.setup(parameters);
  minimizeAllGuiGroupsRecursive(gui);
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
        std::for_each(modPtrs.cbegin(), modPtrs.cend(), [sinkId, &modBids](auto& modPtr) {
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
  std::for_each(fboConfigPtrs.begin(), fboConfigPtrs.end(), [this](const auto& fcptr) {
    if (fcptr->clearOnUpdate) {
      fcptr->fboPtr->getSource().begin();
      ofClear(fcptr->clearColor);
      fcptr->fboPtr->getSource().end();
    }
  });
  
  std::for_each(modPtrs.cbegin(), modPtrs.cend(), [](auto& modPtr) {
    TSGL_START(modPtr->name);
    TS_START(modPtr->name);
    modPtr->update();
    TS_STOP(modPtr->name);
    TSGL_STOP(modPtr->name);
  });
  
  updateSidePanels();
}

// TODO: Could the draw to composite be a Mod that could then forward an FBO?
void Synth::draw() {
  TSGL_START("Synth::draw");
  imageCompositeFbo.begin();
  {
    ofFloatColor backgroundColor = backgroundColorParameter;
    backgroundColor *= backgroundMultiplierParameter; backgroundColor.a = 1.0;
    ofClear(backgroundColor);
    
    size_t i = 0;
    std::for_each(fboConfigPtrs.begin(), fboConfigPtrs.end(), [this, &i](const auto& fcptr) {
      ofEnableBlendMode(fcptr->blendMode);
      float layerAlpha = fboParameters.getFloat(i);
      ++i;
      if (layerAlpha == 0.0) return;
      ofSetColor(ofFloatColor { 1.0, 1.0, 1.0, layerAlpha });
      fcptr->fboPtr->draw(0, 0, imageCompositeFbo.getWidth(), imageCompositeFbo.getHeight());
    });
  }
  imageCompositeFbo.end();

  drawSidePanels();
  
  ofPushMatrix();
  ofTranslate((ofGetWindowWidth() - imageCompositeFbo.getWidth() * compositeScale) / 2.0, (ofGetWindowHeight() - imageCompositeFbo.getHeight() * compositeScale) / 2.0);
  
  ofPushMatrix();
  ofScale(compositeScale, compositeScale);
  ofSetColor(255);
  tonemapShader.begin(toneMapTypeParameter, exposureParameter, gammaParameter, whitePointParameter);
  imageCompositeFbo.draw(0.0, 0.0);
  tonemapShader.end();
  ofPopMatrix();

  // For Mods that draw directly and not on an FBO,
  // for example audio data plots and other debug views
  ofScale(ofGetWindowHeight(), ofGetWindowHeight()); // hack
  std::for_each(modPtrs.cbegin(), modPtrs.cend(), [](auto& modPtr) {
    modPtr->draw();
  });
  
  ofPopMatrix();
  
#ifndef TARGET_OS_IOS
  if (recorder.isRecording()) {
    recorderCompositeFbo.begin();
    imageCompositeFbo.draw(0, 0, recorderCompositeFbo.getWidth(), recorderCompositeFbo.getHeight());
    recorderCompositeFbo.end();
    ofPixels pixels;
    recorderCompositeFbo.readToPixels(pixels);
    recorder.addFrame(pixels);
  }
#endif

  TSGL_STOP("Synth::draw");
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

#ifndef TARGET_OS_IOS
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

  bool handled = std::any_of(modPtrs.cbegin(), modPtrs.cend(), [&key](auto& modPtr) {
    return modPtr->keyPressed(key);
  });
  return handled;
}

void Synth::drawSidePanels() {
  float leftCycleElapsed = (ofGetElapsedTimef() - leftSidePanelLastUpdate) / leftSidePanelTimeoutSecs;
  float rightCycleElapsed = (ofGetElapsedTimef() - rightSidePanelLastUpdate) / rightSidePanelTimeoutSecs;

  // old panels fade out; new panels fade in
  ofBlendMode(OF_BLENDMODE_ALPHA);
  leftPanelCompositeFbo.begin();
  {
    ofSetColor(ofFloatColor { 1.0, 1.0, 1.0, 1.0f - leftCycleElapsed });
    leftPanelFbo.getTarget().draw(0.0, 0.0);
    ofSetColor(ofFloatColor { 1.0, 1.0, 1.0, leftCycleElapsed });
    leftPanelFbo.getSource().draw(0.0, 0.0);
  }
  leftPanelCompositeFbo.end();

  rightPanelCompositeFbo.begin();
  {
    ofSetColor(ofFloatColor { 1.0, 1.0, 1.0, 1.0f - rightCycleElapsed });
    rightPanelFbo.getTarget().draw(0.0, 0.0);
    ofSetColor(ofFloatColor { 1.0, 1.0, 1.0, rightCycleElapsed });
    rightPanelFbo.getSource().draw(0.0, 0.0);
  }
  rightPanelCompositeFbo.end();

  tonemapShader.begin(toneMapTypeParameter, sideExposureParameter, gammaParameter, whitePointParameter);
  ofSetColor(255);
  ofBlendMode(OF_BLENDMODE_DISABLED);
  leftPanelCompositeFbo.draw(0.0, 0.0);
  rightPanelCompositeFbo.draw(ofGetWindowWidth() - sidePanelWidth, 0.0);
  tonemapShader.end();
}

glm::vec2 randomCentralRectOrigin(glm::vec2 rectSize, glm::vec2 bounds) {
  float x = ofRandom(bounds.x / 4.0, bounds.x * 3.0 / 4.0 - rectSize.x);
  float y = ofRandom(bounds.y / 4.0, bounds.y * 3.0 / 4.0 - rectSize.y);
  return { x, y };
}

// target is the outgoing image; source is the incoming one
void Synth::updateSidePanels() {
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

ofParameterGroup& Synth::getFboParameterGroup() {
  if (fboParameters.size() == 0) {
    fboParameters.setName("Layers");
    std::for_each(fboConfigPtrs.begin(), fboConfigPtrs.end(), [this](const auto& fcptr) {
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
  std::for_each(modPtrs.cbegin(), modPtrs.cend(), [this](auto& modPtr) {
    ofParameterGroup& pg = modPtr->getParameterGroup();
    if (pg.size() != 0) parameters.add(pg);
  });
}



} // ofxMarkSynth
