//
//  MultiplyAddMod.hpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 28/10/2025.
//

#pragma once

#include "Mod.hpp"



namespace ofxMarkSynth {



class MultiplyAddMod : public Mod {
  
public:
  MultiplyAddMod(Synth* synthPtr, const std::string& name, const ModConfig&& config);
  void receive(int sinkId, const float& value) override;
  
  static constexpr int SINK_MULTIPLIER = 10;
  static constexpr int SINK_ADDER = 11;
  static constexpr int SINK_FLOAT = 20;
  static constexpr int SOURCE_FLOAT = 30;
  
protected:
  void initParameters() override;
  
private:
  ofParameter<float> multiplierParameter { "Multiplier", 1.0, -4.0, 4.0 };
  ofParameter<float> adderParameter { "Adder", 0.0, -1.0, 1.0 };
};



} // ofxMarkSynth
