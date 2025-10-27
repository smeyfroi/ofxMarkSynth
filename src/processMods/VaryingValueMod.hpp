//
//  VaryingValueMod.hpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 27/10/2025.
//

#pragma once

#include "Mod.hpp"

namespace ofxMarkSynth {



class VaryingValueMod : public Mod {
public:
  VaryingValueMod(const std::string& name, const ModConfig&& config);
  void update() override;
  void receive(int sinkId, const float& value) override;

  static constexpr int SINK_MEAN = 10;
  static constexpr int SINK_VARIANCE = 11;
  static constexpr int SOURCE_FLOAT = 50;
  
protected:
  void initParameters() override;
  
private:
  ofParameter<float> sinkScaleParameter { "Scale", 1.0, 0.0, 10.0 };
  ofParameter<float> meanValueParameter { "Mean", 0.5, 0.0, 1.0 };
  ofParameter<float> varianceParameter { "Variance", 1.0, 0.0, 1.0 };
  ofParameter<float> minParameter { "Min", 0.2, 0.0, 1.0 };
  ofParameter<float> maxParameter { "Max", 0.2, 0.0, 1.0 };
  
  void minMaxChanged(float& value);

};



} // ofxMarkSynth
