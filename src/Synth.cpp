//
//  Synth.cpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#include "Synth.hpp"

namespace ofxMarkSynth {


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

void Synth::configure(ModPtrs&& modPtrs_, FboPtr fboPtr_) {
  modPtrs = std::move(modPtrs_);
  fboPtr = fboPtr_;
  imageCompositeFbo.allocate(fboPtr->getWidth(), fboPtr->getHeight(), GL_RGB);
}

void Synth::update() {
  std::for_each(modPtrs.cbegin(), modPtrs.cend(), [](auto& modPtr) {
    modPtr->update();
  });
}

void Synth::draw() {
  // NOTE: This Mod::draw is for unusual Mods that draw directly and not on an FBO.
  // NOTE: Is that a real thing or should the few that draw directly be refactored?
  std::for_each(modPtrs.cbegin(), modPtrs.cend(), [](auto& modPtr) {
    modPtr->draw();
  });
  
  fboPtr->draw(0, 0, ofGetWindowWidth(), ofGetWindowHeight());
  
  if (recorder.isRecording()) {
    recorderCompositeFbo.clearColorBuffer(ofColor(0, 0, 0));
    recorderCompositeFbo.begin();
    fboPtr->draw(0, 0, ofGetWindowWidth(), ofGetWindowHeight());
    recorderCompositeFbo.end();
    ofPixels pixels;
    recorderCompositeFbo.readToPixels(pixels);
    recorder.addFrame(pixels);
  }
}

bool Synth::keyPressed(int key) {
  if (key == 'S') {
    imageCompositeFbo.clearColorBuffer(ofColor(0, 0, 0));
    imageCompositeFbo.begin();
    fboPtr->draw(0, 0);
    imageCompositeFbo.end();
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
