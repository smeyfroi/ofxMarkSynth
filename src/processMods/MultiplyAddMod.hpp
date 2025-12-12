//
//  MultiplyAddMod.hpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 28/10/2025.
//

#pragma once

#include "Mod.hpp"
#include "ParamController.h"
#include "IntentMapping.hpp"



namespace ofxMarkSynth {



class MultiplyAddMod : public Mod {
  
public:
  MultiplyAddMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config);
  float getAgency() const override;
  void update() override;
  void receive(int sinkId, const float& value) override;
  void applyIntent(const Intent& intent, float strength) override;

  static constexpr int SINK_MULTIPLIER = 10;
  static constexpr int SINK_ADDER = 11;
  static constexpr int SINK_FLOAT = 20;
  static constexpr int SOURCE_FLOAT = 30;
  
protected:
  void initParameters() override;
  
private:
  ofParameter<float> multiplierParameter { "Multiplier", 1.0, -2.0, 2.0 };
  ParamController<float> multiplierController { multiplierParameter };
  ofParameter<float> adderParameter { "Adder", 0.0, -1.0, 1.0 };
  ParamController<float> adderController { adderParameter };
  ofParameter<float> agencyFactorParameter { "AgencyFactor", 1.0, 0.0, 1.0 }; // 0.0 -> No agency; 1.0 -> Global synth agency
};



} // ofxMarkSynth
