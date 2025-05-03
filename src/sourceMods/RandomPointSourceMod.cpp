//
//  RandomPointSourceMod.cpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#include "RandomPointSourceMod.hpp"


namespace ofxMarkSynth {


RandomPointSourceMod::RandomPointSourceMod(const std::string& name, const ModConfig&& config)
: Mod { name, std::move(config) }
{}

void RandomPointSourceMod::initParameters() {
  parameters.add(pointsPerUpdateParameter);
}

void RandomPointSourceMod::update() {
  pointCount += pointsPerUpdateParameter;
  int pointsToCreate = std::floor(pointCount);
  pointCount -= pointsToCreate;
  if (pointsToCreate == 0) return;

  for (int i = 0; i < pointsToCreate; i++) {
    std::for_each(connections[SOURCE_POINTS]->begin(),
                  connections[SOURCE_POINTS]->end(),
                  [&](auto& p) {
      auto& [modPtr, sinkId] = p;
      modPtr->receive(sinkId, createRandomPoint());
    });
  }
}

const glm::vec2 RandomPointSourceMod::createRandomPoint() const {
  return glm::vec2 { ofRandom(), ofRandom() };
}


} // ofxMarkSynth
