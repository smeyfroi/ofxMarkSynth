//
//  Synth.cpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#include "Synth.hpp"

namespace ofxMarkSynth {


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


Synth::Synth() {
  recorder.setup(/*video*/true, /*audio*/false, ofGetWindowSize(), /*fps*/30.0, /*bitrate*/10000);
  recorder.setOverWrite(true);
  auto recordingPath = saveFilePath("");
  std::filesystem::create_directory(recordingPath);
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
  
  imageCompositeFbo.allocate(compositeSize_.x, compositeSize_.y, GL_RGBA);
}

void Synth::update() {
  std::for_each(fboConfigPtrs.begin(), fboConfigPtrs.end(), [this](const auto& fcptr) {
    if (fcptr->clearOnUpdate) {
      fcptr->fboPtr->getSource().begin();
      ofSetColor(fcptr->clearColor);
      ofFill();
      ofDrawRectangle({0.0, 0.0}, fcptr->fboPtr->getWidth(), fcptr->fboPtr->getHeight());
      fcptr->fboPtr->getSource().end();
    }
  });
  
  std::for_each(modPtrs.cbegin(), modPtrs.cend(), [](auto& modPtr) {
    modPtr->update();
  });
}

// TODO: Could the draw to composite be a Mod that could then forward an FBO?
void Synth::draw() {
  imageCompositeFbo.begin();
  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  
  ofSetColor(backgroundColorParameter);
  ofFill();
  ofDrawRectangle({0, 0}, imageCompositeFbo.getWidth(), imageCompositeFbo.getHeight());
  
  size_t i = 0;
  std::for_each(fboConfigPtrs.begin(), fboConfigPtrs.end(), [this, &i](const auto& fcptr) {
    ofEnableBlendMode(fcptr->blendMode);
    ofSetColor(ofFloatColor { 1.0, 1.0, 1.0, fboParameters.getFloat(i) });
    fcptr->fboPtr->draw(0, 0, imageCompositeFbo.getWidth(), imageCompositeFbo.getHeight());
    ++i;
  });

  imageCompositeFbo.end();
  imageCompositeFbo.draw(0, 0, ofGetWindowWidth(), ofGetWindowHeight());
  
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
}

bool Synth::keyPressed(int key) {
  if (key == 'S') {
    ofPixels pixels;
    imageCompositeFbo.readToPixels(pixels);
    ofSaveImage(pixels,
                saveFilePath("snapshot-"+ofGetTimestampString()+".png"),
                OF_IMAGE_QUALITY_BEST);
    return true;
  }
  
  if (key == 'R') {
    if (recorder.isRecording()) {
      recorder.stop();
      ofSetWindowTitle("");
    } else {
      recorder.setOutputPath(saveFilePath("recording-"+ofGetTimestampString()+".mp4"));
      recorder.startCustomRecord();
      ofSetWindowTitle("[Recording]");
    }
  }

  bool handled = std::any_of(modPtrs.cbegin(), modPtrs.cend(), [&](auto& modPtr) {
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
