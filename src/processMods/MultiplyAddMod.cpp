//
//  MultiplyAddMod.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 28/10/2025.
//

#include "MultiplyAddMod.hpp"



namespace ofxMarkSynth {



MultiplyAddMod::MultiplyAddMod(Synth* synthPtr, const std::string& name, const ModConfig&& config)
: Mod { synthPtr, name, std::move(config) }
{
  sinkNameIdMap = {
    { "multiplier", SINK_MULTIPLIER },
    { "adder", SINK_ADDER },
    { "float", SINK_FLOAT }
  };
  sourceNameIdMap = {
    { "float", SOURCE_FLOAT }
  };
}

void MultiplyAddMod::initParameters() {
  parameters.add(multiplierParameter);
  parameters.add(adderParameter);
}

//float MultiplyAddMod::getAgency() const {
//  return Mod::getAgency() * agencyFactorParameter;
//}

void MultiplyAddMod::update() {
  multiplierController.update();
  adderController.update();
}

void MultiplyAddMod::receive(int sinkId, const float& value) {
  switch (sinkId) {
    case SINK_MULTIPLIER:
      multiplierController.updateAuto(value, getAgency());
      break;
    case SINK_ADDER:
      adderController.updateAuto(value, getAgency());
      break;
    case SINK_FLOAT:
      emit(SOURCE_FLOAT, value * multiplierController.value + adderController.value);
      break;
    default:
      ofLogError() << "float receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void MultiplyAddMod::applyIntent(const Intent& intent, float strength) {
  float multI = linearMap(intent.getEnergy(), 0.7f, 1.6f);
  float addI = linearMap(intent.getDensity(), -0.05f, 0.15f);
  multiplierController.updateIntent(multI, strength);
  adderController.updateIntent(addI, strength);
}



} // ofxMarkSynth
