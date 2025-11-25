//
//  RandomFloatSourceMod.cpp
//  example_points
//
//  Created by Steve Meyfroidt on 05/05/2025.
//

#include "RandomFloatSourceMod.hpp"
#include "IntentMapping.hpp"


namespace ofxMarkSynth {


RandomFloatSourceMod::RandomFloatSourceMod(Synth* synthPtr, const std::string& name, ModConfig config, std::pair<float, float> minRange, std::pair<float, float> maxRange, unsigned long randomSeed)
: Mod { synthPtr, name, std::move(config) }
{
  ofSetRandomSeed(randomSeed);
  minParameter.setMin(minRange.first);
  minParameter.setMax(minRange.second);
  maxParameter.setMin(maxRange.first);
  maxParameter.setMax(maxRange.second);
  
  sourceNameIdMap = {
    { "Float", SOURCE_FLOAT }
  };
  
  sourceNameControllerPtrMap = {
    { floatsPerUpdateParameter.getName(), &floatsPerUpdateController },
    { minParameter.getName(), &minController },
    { maxParameter.getName(), &maxController }
  };
}

void RandomFloatSourceMod::initParameters() {
  parameters.add(floatsPerUpdateParameter);
  parameters.add(minParameter);
  parameters.add(maxParameter);
}

void RandomFloatSourceMod::update() {
  syncControllerAgencies();
  floatsPerUpdateController.update();
  minController.update();
  maxController.update();
  
  floatCount += floatsPerUpdateController.value;
  int floatsToCreate = std::floor(floatCount);
  floatCount -= floatsToCreate;
  if (floatsToCreate == 0) return;

  for (int i = 0; i < floatsToCreate; i++) {
    emit(SOURCE_FLOAT, createRandomFloat());
  }
}

const float RandomFloatSourceMod::createRandomFloat() const {
  return ofRandom(minController.value, maxController.value);
}

void RandomFloatSourceMod::applyIntent(const Intent& intent, float strength) {

  // Density → floatsPerUpdate
  floatsPerUpdateController.updateIntent(exponentialMap(intent.getDensity(), floatsPerUpdateController, 0.5f), strength);
  
  // Energy → range width (higher energy = wider range, centered around midpoint)
  float currentMid = (minController.getManualMin() + maxController.getManualMax()) * 0.5f;
  float fullRange = maxController.getManualMax() - minController.getManualMin();
  float targetRange = linearMap(intent.getEnergy(), 0.2f * fullRange, fullRange);
  float halfRange = targetRange * 0.5f;
  minController.updateIntent(std::max(minController.getManualMin(), currentMid - halfRange), strength);
  maxController.updateIntent(std::min(maxController.getManualMax(), currentMid + halfRange), strength);
}


} // ofxMarkSynth
