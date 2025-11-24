//
//  StaticTextSourceMod.hpp
//  ofxMarkSynth
//
//  Created by Steve Meyfroidt on 18/11/2025.
//

#pragma once

#include "Mod.hpp"
#include "ParamController.h"



namespace ofxMarkSynth {



class StaticTextSourceMod : public Mod {

public:
  StaticTextSourceMod(Synth* synthPtr, const std::string& name, ModConfig config);
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
  
  float startTime;
  
  void onTextChanged(std::string& text);
};



} // ofxMarkSynth
