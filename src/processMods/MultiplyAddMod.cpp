//
//  MultiplyAddMod.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 28/10/2025.
//

#include "MultiplyAddMod.hpp"
#include "core/IntentMapper.hpp"



namespace ofxMarkSynth {



MultiplyAddMod::MultiplyAddMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config)
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
  
  registerControllerForSource(multiplierParameter, multiplierController);
  registerControllerForSource(adderParameter, adderController);
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
  syncControllerAgencies();
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
  IntentMap im(intent);
  
  im.E().exp(multiplierController, strength);

  // Weighted blend: density (60%) + granularity (40%)
  float combinedAdd = im.D().get() * 0.6f + im.G().get() * 0.4f;
  float addI = exponentialMap(combinedAdd, adderController);
  adderController.updateIntent(addI, strength, "D*.6+G -> exp");
}



} // ofxMarkSynth
