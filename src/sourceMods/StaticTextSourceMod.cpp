//
//  StaticTextSourceMod.cpp
//  ofxMarkSynth
//
//  Created by Steve Meyfroidt on 18/11/2025.
//

#include "StaticTextSourceMod.hpp"
#include "Synth.hpp"



namespace ofxMarkSynth {



StaticTextSourceMod::StaticTextSourceMod(Synth* synthPtr, const std::string& name, ModConfig config,
                                         const std::string& text)
: Mod { synthPtr, name, std::move(config) },
  staticText { text }
{
  sourceNameIdMap = {
    { "text", SOURCE_TEXT }
  };
  
  startTime = ofGetElapsedTimef();
}

void StaticTextSourceMod::initParameters() {
  parameters.add(emitOnceParameter);
  parameters.add(delayParameter);
}

void StaticTextSourceMod::update() {
  if (hasEmitted && emitOnceParameter) {
    return;
  }
  
  float elapsedTime = ofGetElapsedTimef() - startTime;
  if (elapsedTime < delayParameter) return;
  
  emit(SOURCE_TEXT, staticText);
  hasEmitted = true;
  
  ofLogNotice("StaticTextSourceMod") << "Emitted text: '" << staticText << "'";
}



} // ofxMarkSynth
