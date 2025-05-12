//
//  Synth.cpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#include "Synth.hpp"

namespace ofxMarkSynth {


void Synth::configure(std::unique_ptr<ModPtrs> modPtrsPtr_, std::shared_ptr<PingPongFbo> fboPtr_) {
  modPtrsPtr = std::move(modPtrsPtr_);
  fboPtr = fboPtr_;
}

void Synth::update() {
  std::for_each(modPtrsPtr->cbegin(), modPtrsPtr->cend(), [](auto& modPtr) {
    modPtr->update();
  });
}

void Synth::draw() {
  std::for_each(modPtrsPtr->cbegin(), modPtrsPtr->cend(), [](auto& modPtr) {
    modPtr->draw();
  });
  fboPtr->draw(0, 0, ofGetWindowWidth(), ofGetWindowHeight());
}

bool Synth::keyPressed(int key) {
  if (key == 'S') {
    ofPixels pixels;
    fboPtr->getSource().readToPixels(pixels);
    ofSaveImage(pixels,
                ofFilePath::getUserHomeDir()
                +"/Documents/MarkSynth/snapshot-"
                +ofGetTimestampString()
                +".png",
                OF_IMAGE_QUALITY_BEST);
    return true;
  }

  bool handled = std::any_of(modPtrsPtr->cbegin(), modPtrsPtr->cend(), [&](auto& modPtr) {
    return modPtr->keyPressed(key);
  });
  return handled;
}

ofParameterGroup& Synth::getParameterGroup(const std::string& groupName) {
  ofParameterGroup parameterGroup = parameters;
  if (parameters.size() == 0) {
    parameters.setName(groupName);
    std::for_each(modPtrsPtr->cbegin(), modPtrsPtr->cend(), [&parameterGroup](auto& modPtr) {
      parameterGroup.add(modPtr->getParameterGroup());
    });
  }
  return parameters;
}


} // ofxMarkSynth
