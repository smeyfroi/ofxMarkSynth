//
//  Synth.cpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#include "Synth.hpp"
#include "ofxTimeMeasurements.h"

namespace ofxMarkSynth {


void PixelsToFile::save(const std::string& filepath_, ofPixels&& pixels_)
{
  if (!isReady) return;
  isReady = false;
  pixels = pixels_;
  filepath = filepath_;
  startThread();
}

void PixelsToFile::threadedFunction() {
  ofLogNotice() << "Saving drawing to " << filepath;
  ofSaveImage(pixels, filepath, OF_IMAGE_QUALITY_BEST);
  isReady = true;
  ofLogNotice() << "Done saving drawing to " << filepath;
}


// See ofFbo.cpp #allocate
void allocateFbo(FboPtr fboPtr, glm::vec2 size, GLint internalFormat, int wrap) {
  ofFboSettings settings { nullptr };
  settings.wrapModeVertical = wrap;
  settings.wrapModeHorizontal = wrap;
  settings.width = size.x;
  settings.height = size.y;
  settings.internalformat = internalFormat;
  settings.numSamples = 0;
  settings.useDepth = true;
  settings.useStencil = true;
  settings.textureTarget = ofGetUsingArbTex() ? GL_TEXTURE_RECTANGLE_ARB : GL_TEXTURE_2D;
  fboPtr->allocate(settings);
}

void addFboConfigPtr(FboConfigPtrs& fboConfigPtrs, std::string name, FboPtr fboPtr, glm::vec2 size, GLint internalFormat, int wrap, ofFloatColor clearColor, bool clearOnUpdate, ofBlendMode blendMode) {
  allocateFbo(fboPtr, size, internalFormat, wrap);
//  fboPtr->getSource().clearColorBuffer(clearColor);
  // Don't know why clearColorBuffer doesn't work
  fboPtr->getSource().begin();
  ofSetColor(clearColor);
  ofFill();
  ofDrawRectangle(0, 0, fboPtr->getWidth(), fboPtr->getHeight());
  fboPtr->getSource().end();

  fboConfigPtrs.emplace_back(std::make_shared<FboConfig>(name, fboPtr, clearColor, clearOnUpdate, blendMode));
}

std::string saveFilePath(std::string filename) {
  return ofFilePath::getUserHomeDir()+"/Documents/MarkSynth/"+filename;
}

constexpr std::string SETTINGS_FOLDER_NAME = "settings";
constexpr std::string SNAPSHOTS_FOLDER_NAME = "drawings";
constexpr std::string VIDEOS_FOLDER_NAME = "drawing-recordings";


Synth::Synth(std::string name_) :
name { name_ }
{
  std::filesystem::create_directory(saveFilePath(SETTINGS_FOLDER_NAME+"/"+name));
  std::filesystem::create_directory(saveFilePath(SNAPSHOTS_FOLDER_NAME+"/"+name));
  std::filesystem::create_directory(saveFilePath(VIDEOS_FOLDER_NAME+"/"+name));

  recorder.setup(/*video*/true, /*audio*/false, ofGetWindowSize(), /*fps*/30.0, /*bitrate*/10000);
  recorder.setOverWrite(true);
  recorder.setFFmpegPathToAddonsPath();
  recorder.setInputPixelFormat(OF_IMAGE_COLOR);
  recorderCompositeFbo.allocate(ofGetWindowWidth(), ofGetWindowHeight(), GL_RGB);
}

Synth::~Synth() {
  if (recorder.isRecording()) recorder.stop();
}

void Synth::configure(FboConfigPtrs&& fboConfigPtrs_, ModPtrs&& modPtrs_, glm::vec2 compositeSize_) {
  if (std::any_of(fboConfigPtrs.begin(), fboConfigPtrs.end(), [this](const auto& fcptr) {
    return fcptr->fboPtr->getSource().isAllocated();
  })) ofLogError() << "Unallocated FBO";
  fboConfigPtrs = std::move(fboConfigPtrs_);
  
  modPtrs = std::move(modPtrs_);
  
  imageCompositeFbo.allocate(compositeSize_.x, compositeSize_.y, GL_RGB);

  parameters = getParameterGroup("Synth");
  gui.setup(parameters);
  minimizeAllGuiGroupsRecursive(gui);
}

void Synth::update() {
  std::for_each(fboConfigPtrs.begin(), fboConfigPtrs.end(), [this](const auto& fcptr) {
    if (fcptr->clearOnUpdate) {
      fcptr->fboPtr->getSource().begin();
      ofEnableBlendMode(OF_BLENDMODE_DISABLED);
      ofSetColor(fcptr->clearColor);
      ofFill();
      ofDrawRectangle({0.0, 0.0}, fcptr->fboPtr->getWidth(), fcptr->fboPtr->getHeight());
      fcptr->fboPtr->getSource().end();
    }
  });
  
  std::for_each(modPtrs.cbegin(), modPtrs.cend(), [](auto& modPtr) {
    TS_SCOPE(modPtr->name);
    TSGL_START(modPtr->name);
    modPtr->update();
    TSGL_STOP(modPtr->name);
  });
}

// TODO: Could the draw to composite be a Mod that could then forward an FBO?
void Synth::draw() {
  TSGL_START("Synth::draw");
  imageCompositeFbo.begin();
  ofSetColor(backgroundColorParameter);
  ofFill();
  ofDrawRectangle(0.0, 0.0, imageCompositeFbo.getWidth(), imageCompositeFbo.getHeight());
  
  size_t i = 0;
  std::for_each(fboConfigPtrs.begin(), fboConfigPtrs.end(), [this, &i](const auto& fcptr) {
    ofEnableBlendMode(fcptr->blendMode);
    ofSetColor(ofFloatColor { 1.0, 1.0, 1.0, fboParameters.getFloat(i) });
    fcptr->fboPtr->draw(0, 0, imageCompositeFbo.getWidth(), imageCompositeFbo.getHeight());
    ++i;
  });
  
  imageCompositeFbo.end();
  imageCompositeFbo.draw(0.0, 0.0, ofGetWindowWidth(), ofGetWindowHeight());
  
  // NOTE: This Mod::draw is for Mods that draw directly and not on an FBO,
  // for example audio data plots and other debug views
  std::for_each(modPtrs.cbegin(), modPtrs.cend(), [](auto& modPtr) {
    modPtr->draw();
  });
  
  if (recorder.isRecording()) {
    recorderCompositeFbo.begin();
    imageCompositeFbo.draw(0, 0, recorderCompositeFbo.getWidth(), recorderCompositeFbo.getHeight());
    recorderCompositeFbo.end();
    ofPixels pixels;
    recorderCompositeFbo.readToPixels(pixels);
    recorder.addFrame(pixels);
  }
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
    if (pixelsToFile.isReady) {
      std::string filepath = saveFilePath(SNAPSHOTS_FOLDER_NAME+"/"+name+"/drawing-"+ofGetTimestampString()+".png");
      ofLogNotice() << "Fetch drawing to save to " << filepath;
      ofPixels pixels;
      imageCompositeFbo.readToPixels(pixels);
      pixelsToFile.save(filepath, std::move(pixels));
    }
    return true;
  }

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

  bool handled = std::any_of(modPtrs.cbegin(), modPtrs.cend(), [&key](auto& modPtr) {
    return modPtr->keyPressed(key);
  });
  return handled;
}

void Synth::minimizeAllGuiGroupsRecursive(ofxGuiGroup& guiGroup) {
  for (int i = 0; i < guiGroup.getNumControls(); ++i) {
    auto control = guiGroup.getControl(i);
    if (auto childGuiGroup = dynamic_cast<ofxGuiGroup*>(control)) {
      childGuiGroup->minimize();
      minimizeAllGuiGroupsRecursive(*childGuiGroup);
    }
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

ofParameterGroup& Synth::getParameterGroup(const std::string& groupName) {
  if (parameters.size() == 0) {
    parameters.setName(groupName);
    parameters.add(backgroundColorParameter);
    parameters.add(getFboParameterGroup());
    std::for_each(modPtrs.cbegin(), modPtrs.cend(), [this](auto& modPtr) {
      parameters.add(modPtr->getParameterGroup());
    });
  }
  return parameters;
}


} // ofxMarkSynth
