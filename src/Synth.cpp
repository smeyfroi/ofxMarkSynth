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
  PingPongFbo fbo;
  fboPtr->allocate(settings);
  fboPtr->getSource().clearColorBuffer(ofFloatColor(0.0, 0.0, 0.0, 0.0));
}


auto saveFilePath(std::string filename) {
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

void Synth::configure(ModPtrs&& modPtrs_, FboConfigPtrs&& fboConfigPtrs_, glm::vec2 compositeSize_) {
  modPtrs = std::move(modPtrs_);
  
  if (std::any_of(fboConfigPtrs.begin(), fboConfigPtrs.end(), [this](const auto& fcptr) {
    return fcptr->fboPtr->getSource().isAllocated();
  })) ofLogError() << "Unallocated FBO";
  fboConfigPtrs = std::move(fboConfigPtrs_);
  
  imageCompositeFbo.allocate(compositeSize_.x, compositeSize_.y, GL_RGBA);
}

void Synth::update() {
  
  std::for_each(fboConfigPtrs.begin(), fboConfigPtrs.end(), [this](const auto& fcptr) {
    if (fcptr->clearColorPtr) {
      fcptr->fboPtr->getSource().clearColorBuffer(*(fcptr->clearColorPtr));
    }
    fcptr->fboPtr->draw(0, 0, imageCompositeFbo.getWidth(), imageCompositeFbo.getHeight());
  });

  std::for_each(modPtrs.cbegin(), modPtrs.cend(), [](auto& modPtr) {
    modPtr->update();
  });
}

// TODO: Could the draw to composite be a Mod that could then forward an FBO?
void Synth::draw() {
  imageCompositeFbo.begin();
  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  ofSetColor(ofFloatColor { 0.0, 0.0, 0.0, 1.0 });
  ofFill();
  ofDrawRectangle({0, 0}, imageCompositeFbo.getWidth(), imageCompositeFbo.getHeight());
  ofSetColor(ofFloatColor { 1.0, 1.0, 1.0, 1.0 });
  std::for_each(fboConfigPtrs.begin(), fboConfigPtrs.end(), [this](const auto& fcptr) {
    fcptr->fboPtr->draw(0, 0, imageCompositeFbo.getWidth(), imageCompositeFbo.getHeight());
  });
  imageCompositeFbo.end();

  imageCompositeFbo.draw(0, 0, ofGetWindowWidth(), ofGetWindowHeight());
  
  // NOTE: This Mod::draw is for unusual Mods that draw directly and not on an FBO,
  // for example audio data plots or other debug views
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

ofParameterGroup& Synth::getParameterGroup(const std::string& groupName) {
  ofParameterGroup parameterGroup = parameters;
  if (parameters.size() == 0) {
    parameters.setName(groupName);
    std::for_each(modPtrs.cbegin(), modPtrs.cend(), [&parameterGroup](auto& modPtr) {
      parameterGroup.add(modPtr->getParameterGroup());
    });
  }
  return parameters;
}


} // ofxMarkSynth
