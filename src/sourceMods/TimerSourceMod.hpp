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
  void receive(int sinkId, const float& value) override;
  void applyIntent(const Intent& intent, float strength) override;
  
  static constexpr int SOURCE_TICK = 1;

  static constexpr int SINK_INTERVAL = 10;
  static constexpr int SINK_ENABLED = 11;
  static constexpr int SINK_ONE_SHOT = 12;
  static constexpr int SINK_TIME_TO_NEXT = 13;
  static constexpr int SINK_START = 14;
  static constexpr int SINK_STOP = 15;
  static constexpr int SINK_RESET = 16;
  static constexpr int SINK_TRIGGER_NOW = 17;
  
protected:
  void initParameters() override;
  
private:
  static constexpr float MIN_INTERVAL = 0.01f;

  ofParameter<float> intervalParameter { "Interval", 1.0, MIN_INTERVAL, 10.0 };
  ParamController<float> intervalController { intervalParameter };
  ofParameter<bool> enabledParameter { "Enabled", true };
  ofParameter<bool> oneShotParameter { "OneShot", false };
  ofParameter<float> timeToNextParameter { "TimeToNext", 0.0f, 0.0f, 10.0f };
  
  float nextFireTime { 0.0f };
  bool hasFired { false };
};



} // ofxMarkSynth
