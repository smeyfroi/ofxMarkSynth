//
//  TimerSourceMod.hpp
//  ofxMarkSynth
//
//  Created by Steve Meyfroidt on 18/11/2025.
//

#pragma once

#include "Mod.hpp"
#include "ParamController.h"



namespace ofxMarkSynth {



class TimerSourceMod : public Mod {

public:
  TimerSourceMod(Synth* synthPtr, const std::string& name, ModConfig config);
  
  void update() override;
  void applyIntent(const Intent& intent, float strength) override;
  
  static constexpr int SOURCE_TICK = 1;
  
protected:
  void initParameters() override;
  
private:
  ofParameter<float> intervalParameter { "Interval", 1.0, 0.01, 10.0 };
  ParamController<float> intervalController { intervalParameter };
  ofParameter<bool> enabledParameter { "Enabled", true };
  
  float lastEmitTime { 0.0f };
};



} // ofxMarkSynth
