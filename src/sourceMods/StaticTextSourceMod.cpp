//
//  StaticTextSourceMod.cpp
//  ofxMarkSynth
//
//  Created by Steve Meyfroidt on 18/11/2025.
//

#include "StaticTextSourceMod.hpp"
#include "Synth.hpp"



namespace ofxMarkSynth {



StaticTextSourceMod::StaticTextSourceMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config)
: Mod { synthPtr, name, std::move(config) }
{
  sourceNameIdMap = {
    { "Text", SOURCE_TEXT }
  };
  
  startTime = ofGetElapsedTimef();
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
  
  float elapsedTime = ofGetElapsedTimef() - startTime;
  if (elapsedTime < delayParameter) return;
  
  emit(SOURCE_TEXT, textParameter.get());
  hasEmitted = true;
//  ofLogVerbose("StaticTextSourceMod") << "Emitted text: '" << textParameter.get() << "'";
}

void StaticTextSourceMod::onTextChanged(std::string& text) {
  hasEmitted = false;
  ofLogNotice("StaticTextSourceMod") << "Text changed to: '" << text << "', will re-emit";
}



} // ofxMarkSynth
