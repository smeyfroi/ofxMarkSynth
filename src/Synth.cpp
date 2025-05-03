//
//  Synth.cpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#include "Synth.hpp"

namespace ofxMarkSynth {


void Synth::configure(std::unique_ptr<ModPtrs> modPtrsPtr_) {
  modPtrsPtr = std::move(modPtrsPtr_);
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
