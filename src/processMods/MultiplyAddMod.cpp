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

void MultiplyAddMod::receive(int sinkId, const float& value) {
  switch (sinkId) {
    case SINK_MULTIPLIER:
      multiplierParameter = value;
      break;
    case SINK_ADDER:
      adderParameter = value;
      break;
    case SINK_FLOAT:
      emit(SOURCE_FLOAT, value * multiplierParameter + adderParameter);
      break;
    default:
      ofLogError() << "float receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}



} // ofxMarkSynth
