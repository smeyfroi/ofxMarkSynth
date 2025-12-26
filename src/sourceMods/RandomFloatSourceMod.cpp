//
//  RandomFloatSourceMod.cpp
//  example_points
//
//  Created by Steve Meyfroidt on 05/05/2025.
//

#include "RandomFloatSourceMod.hpp"
#include "IntentMapping.hpp"
#include "IntentMapper.hpp"


namespace ofxMarkSynth {


RandomFloatSourceMod::RandomFloatSourceMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config, std::pair<float, float> minRange, std::pair<float, float> maxRange, unsigned long randomSeed)
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
  
  registerControllerForSource(floatsPerUpdateParameter, floatsPerUpdateController);
  registerControllerForSource(minParameter, minController);
  registerControllerForSource(maxParameter, maxController);
}

void RandomFloatSourceMod::initParameters() {
  parameters.add(floatsPerUpdateParameter);
  parameters.add(minParameter);
  parameters.add(maxParameter);
  parameters.add(agencyFactorParameter);
}

float RandomFloatSourceMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
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
  IntentMap im(intent);

  im.D().exp(floatsPerUpdateController, strength, 0.5f);
  
  // Energy expands range symmetrically around midpoint
  float currentMid = (minController.getManualMin() + maxController.getManualMax()) * 0.5f;
  float fullRange = maxController.getManualMax() - minController.getManualMin();
  float targetRange = linearMap(im.E().get(), 0.2f * fullRange, fullRange);
  float halfRange = targetRange * 0.5f;
  minController.updateIntent(std::max(minController.getManualMin(), currentMid - halfRange), strength, "E -> range");
  maxController.updateIntent(std::min(maxController.getManualMax(), currentMid + halfRange), strength, "E -> range");
}


} // ofxMarkSynth
