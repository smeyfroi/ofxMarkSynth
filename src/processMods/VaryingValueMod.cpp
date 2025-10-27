//
//  VaryingValueMod.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 27/10/2025.
//

#include "VaryingValueMod.hpp"

namespace ofxMarkSynth {



VaryingValueMod::VaryingValueMod(const std::string& name, const ModConfig&& config)
: Mod { name, std::move(config) }
{
}

void VaryingValueMod::initParameters() {
  parameters.add(sinkScaleParameter);
  parameters.add(meanValueParameter);
  parameters.add(varianceParameter);
  parameters.add(minParameter);
  parameters.add(maxParameter);
  minParameter.addListener(this, &VaryingValueMod::minMaxChanged);
  maxParameter.addListener(this, &VaryingValueMod::minMaxChanged);
}

void VaryingValueMod::update() {
  float stdDev = varianceParameter.get() * (maxParameter - minParameter);
  float sampledValue = of::random::normal<float>(meanValueParameter.get(), stdDev);
  float value = ofClamp(sampledValue, minParameter.get(), maxParameter.get());
  emit(SOURCE_FLOAT, value);
}

void VaryingValueMod::minMaxChanged(float& value) {
  meanValueParameter.setMin(minParameter.get());
  meanValueParameter.setMax(maxParameter.get());
  minParameter.setMax(maxParameter.get());
  maxParameter.setMin(minParameter.get());
  maxParameter.setMax(std::fminf(10.0f, maxParameter.get() * 4.0));
}

void VaryingValueMod::receive(int sinkId, const float& v) {
  switch (sinkId) {
    case SINK_MEAN:
      meanValueParameter = v * sinkScaleParameter.get();
      break;
    case SINK_VARIANCE:
      varianceParameter = v;
      break;
    default:
      ofLogError() << "float receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}



} // ofxMarkSynth
