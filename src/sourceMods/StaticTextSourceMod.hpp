//
//  StaticTextSourceMod.hpp
//  ofxMarkSynth
//
//  Created by Steve Meyfroidt on 18/11/2025.
//

#pragma once

#include "core/Mod.hpp"
#include "core/ParamController.h"



namespace ofxMarkSynth {



class StaticTextSourceMod : public Mod {

public:
  StaticTextSourceMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config);
  ~StaticTextSourceMod();
  
  void update() override;
  
  static constexpr int SOURCE_TEXT = 1;
  
protected:
  void initParameters() override;
  
private:
  bool hasEmitted { false };
  
  ofParameter<std::string> textParameter { "Text", "" };
  ofParameter<bool> emitOnceParameter { "EmitOnce", true };
  ofParameter<float> delayParameter { "Delay", 0.0, 0.0, 10.0 };
  
  float accumulatedTime { 0.0f };  // Accumulated time since last reset (only counts when update() is called)
  
  void onTextChanged(std::string& text);
};



} // ofxMarkSynth
