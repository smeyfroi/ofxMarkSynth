//
//  VaryingValueMod.hpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 27/10/2025.
//

//#pragma once
//
//#include "Mod.hpp"
//#include "ParamController.h"
//
//namespace ofxMarkSynth {
//
//
//
//class VaryingValueMod : public Mod {
//public:
//  VaryingValueMod(Synth* synthPtr, const std::string& name, const ModConfig&& config);
//  void update() override;
//  void receive(int sinkId, const float& value) override;
//  void applyIntent(const Intent& intent, float strength) override;
//
//  static constexpr int SINK_MEAN = 10;
//  static constexpr int SINK_VARIANCE = 11;
//  static constexpr int SOURCE_FLOAT = 50;
//  
//protected:
//  void initParameters() override;
//  
//private:
//  ofParameter<float> sinkScaleParameter { "Scale", 1.0, 0.0, 10.0 };
//  ParamController<float> sinkScaleController { sinkScaleParameter };
//  ofParameter<float> meanValueParameter { "Mean", 0.5, 0.0, 1.0 };
//  ParamController<float> meanValueController { meanValueParameter };
//  ofParameter<float> varianceParameter { "Variance", 1.0, 0.0, 1.0 };
//  ParamController<float> varianceController { varianceParameter };
//  ofParameter<float> minParameter { "Min", 0.2, 0.0, 1.0 };
//  ParamController<float> minController { minParameter };
//  ofParameter<float> maxParameter { "Max", 0.2, 0.0, 1.0 };
//  ParamController<float> maxController { maxParameter };
//  
//  void minMaxChanged(float& value);
//
//};
//
//
//
//} // ofxMarkSynth
