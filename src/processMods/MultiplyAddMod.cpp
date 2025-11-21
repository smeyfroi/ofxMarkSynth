//
//  MultiplyAddMod.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 28/10/2025.
//

#include "MultiplyAddMod.hpp"



namespace ofxMarkSynth {



MultiplyAddMod::MultiplyAddMod(Synth* synthPtr, const std::string& name, ModConfig config)
: Mod { synthPtr, name, std::move(config) }
{
  sinkNameIdMap = {
    { multiplierParameter.getName(), SINK_MULTIPLIER },
    { adderParameter.getName(), SINK_ADDER },
    { "float", SINK_FLOAT }
  };
  sourceNameIdMap = {
    { "float", SOURCE_FLOAT }
  };
  
  sourceNameControllerPtrMap = {
    { multiplierParameter.getName(), &multiplierController },
    { adderParameter.getName(), &adderController }
  };
}

void MultiplyAddMod::initParameters() {
  parameters.add(multiplierParameter);
  parameters.add(adderParameter);
  parameters.add(agencyFactorParameter);
}

float MultiplyAddMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

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
      ofLogError("MultiplyAddMod") << "Float receive for unknown sinkId " << sinkId;
  }
}

void MultiplyAddMod::applyIntent(const Intent& intent, float strength) {
  if (strength < 0.01) return;
  
  // Energy → multiply (higher energy = higher multiplier)
  float multI = exponentialMap(intent.getEnergy(), multiplierController);
  multiplierController.updateIntent(multI, strength);

  // Density + Granularity → add (combined influence on offset)
  float combinedAdd = intent.getDensity() * 0.6f + intent.getGranularity() * 0.4f;
  float addI = exponentialMap(combinedAdd, adderController);
  adderController.updateIntent(addI, strength);
}



} // ofxMarkSynth
