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
  StaticTextSourceMod(Synth* synthPtr, const std::string& name, ModConfig config,
                      const std::string& text);
  
  void update() override;
  
  static constexpr int SOURCE_TEXT = 1;
  
protected:
  void initParameters() override;
  
private:
  std::string staticText;
  bool hasEmitted { false };
  
  ofParameter<bool> emitOnceParameter { "Emit Once", true };
  ofParameter<float> delayParameter { "Delay", 0.0, 0.0, 10.0 };
  
  float startTime;
};



} // ofxMarkSynth
