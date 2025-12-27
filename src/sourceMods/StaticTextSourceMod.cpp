//
//  StaticTextSourceMod.cpp
//  ofxMarkSynth
//
//  Created by Steve Meyfroidt on 18/11/2025.
//

#include "StaticTextSourceMod.hpp"
#include "core/Synth.hpp"



namespace ofxMarkSynth {



StaticTextSourceMod::StaticTextSourceMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config)
: Mod { synthPtr, name, std::move(config) }
{
  sourceNameIdMap = {
    { "Text", SOURCE_TEXT }
  };
}

StaticTextSourceMod::~StaticTextSourceMod() {
  textParameter.removeListener(this, &StaticTextSourceMod::onTextChanged);
}

void StaticTextSourceMod::initParameters() {
  parameters.add(textParameter);
  parameters.add(emitOnceParameter);
  parameters.add(delayParameter);
  
  textParameter.addListener(this, &StaticTextSourceMod::onTextChanged);
}

void StaticTextSourceMod::update() {
  syncControllerAgencies();
  if (hasEmitted && emitOnceParameter) {
    return;
  }
  
  // Accumulate time only when update() is called (i.e., synth is unpaused and running)
  // This is naturally pause-aware because Synth only calls Mod::update() when not paused
  accumulatedTime += ofGetLastFrameTime();
  if (accumulatedTime < delayParameter) return;
  
  emit(SOURCE_TEXT, textParameter.get());
  hasEmitted = true;
}

void StaticTextSourceMod::onTextChanged(std::string& text) {
  hasEmitted = false;
  accumulatedTime = 0.0f;  // Reset delay timer when text changes
}



} // ofxMarkSynth
