//
//  RandomFloatSourceMod.cpp
//  example_points
//
//  Created by Steve Meyfroidt on 05/05/2025.
//

#include "RandomFloatSourceMod.hpp"


namespace ofxMarkSynth {


RandomFloatSourceMod::RandomFloatSourceMod(Synth* synthPtr, const std::string& name, const ModConfig&& config, std::pair<float, float> minRange, std::pair<float, float> maxRange, unsigned long randomSeed)
: Mod { synthPtr, name, std::move(config) }
{
  ofSetRandomSeed(randomSeed);
  minParameter.setMin(minRange.first);
  minParameter.setMax(minRange.second);
  maxParameter.setMin(maxRange.first);
  maxParameter.setMax(maxRange.second);
}

void RandomFloatSourceMod::initParameters() {
  parameters.add(floatsPerUpdateParameter);
  parameters.add(minParameter);
  parameters.add(maxParameter);
}

void RandomFloatSourceMod::update() {
  floatCount += floatsPerUpdateParameter;
  int floatsToCreate = std::floor(floatCount);
  floatCount -= floatsToCreate;
  if (floatsToCreate == 0) return;

  for (int i = 0; i < floatsToCreate; i++) {
    emit(SOURCE_FLOAT, createRandomFloat());
  }
}

const float RandomFloatSourceMod::createRandomFloat() const {
  return ofRandom(minParameter, maxParameter);
}


} // ofxMarkSynth
