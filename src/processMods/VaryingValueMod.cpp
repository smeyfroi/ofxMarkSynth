//
//  VaryingValueMod.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 27/10/2025.
//

//#include "VaryingValueMod.hpp"
//#include "IntentMapping.hpp"
//
//namespace ofxMarkSynth {
//
//
//
//VaryingValueMod::VaryingValueMod(Synth* synthPtr, const std::string& name, ModConfig config)
//: Mod { synthPtr, name, std::move(config) }
//{
//  sinkNameIdMap = {
//    { "mean", SINK_MEAN },
//    { "variance", SINK_VARIANCE }
//  };
//  sourceNameIdMap = {
//    { "float", SOURCE_FLOAT }
//  };
//}
//
//void VaryingValueMod::initParameters() {
//  parameters.add(sinkScaleParameter);
//  parameters.add(meanValueParameter);
//  parameters.add(varianceParameter);
//  parameters.add(minParameter);
//  parameters.add(maxParameter);
//  minParameter.addListener(this, &VaryingValueMod::minMaxChanged);
//  maxParameter.addListener(this, &VaryingValueMod::minMaxChanged);
//}
//
//void VaryingValueMod::update() {
//  sinkScaleController.update();
//  meanValueController.update();
//  varianceController.update();
//  minController.update();
//  maxController.update();
//  float stdDev = varianceParameter.get() * (maxController.value - minController.value);
//  float sampledValue = of::random::normal<float>(meanValueParameter.get(), stdDev);
//  float value = ofClamp(sampledValue, minParameter.get(), maxParameter.get());
//  emit(SOURCE_FLOAT, value);
//}
//
//void VaryingValueMod::minMaxChanged(float& value) {
//  meanValueParameter.setMin(minParameter.get());
//  meanValueParameter.setMax(maxParameter.get());
//  minParameter.setMax(maxParameter.get());
//  maxParameter.setMin(minParameter.get());
//  maxParameter.setMax(std::fminf(10.0f, maxParameter.get() * 4.0));
//}
//
//void VaryingValueMod::receive(int sinkId, const float& v) {
//  switch (sinkId) {
//    case SINK_MEAN:
//      meanValueParameter = v * sinkScaleParameter.get();
//      break;
//    case SINK_VARIANCE:
//      varianceParameter = v;
//      break;
//    default:
//      ofLogError() << "float receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
//  }
//}
//
//void VaryingValueMod::applyIntent(const Intent& intent, float strength) {
//  varianceController.updateIntent(exponentialMap(intent.getChaos(), 0.2f, 1.0f, 2.0f), strength);
//  float range = maxController.value - minController.value;
//  meanValueController.updateIntent(minController.value + linearMap(intent.getDensity(), 0.3f, 0.7f) * range, strength);
//}
//
//
//
//} // ofxMarkSynth
