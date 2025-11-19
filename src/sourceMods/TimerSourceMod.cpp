//
//  TimerSourceMod.cpp
//  ofxMarkSynth
//
//  Created by Steve Meyfroidt on 18/11/2025.
//

#include "TimerSourceMod.hpp"
#include "Synth.hpp"
#include "IntentMapping.hpp"
#include "ofUtils.h"



namespace ofxMarkSynth {



TimerSourceMod::TimerSourceMod(Synth* synthPtr, const std::string& name, ModConfig config)
: Mod { synthPtr, name, std::move(config) }
{
  sourceNameIdMap = {
    { "tick", SOURCE_TICK }
  };
  
  sourceNameControllerPtrMap = {
    { intervalParameter.getName(), &intervalController }
  };
  
  lastEmitTime = ofGetElapsedTimef();
}

void TimerSourceMod::initParameters() {
  parameters.add(intervalParameter);
  parameters.add(enabledParameter);
}

void TimerSourceMod::update() {
  intervalController.update();
  
  if (!enabledParameter) {
    return;
  }
  
  float currentTime = ofGetElapsedTimef();
  if (currentTime - lastEmitTime >= intervalController.value) {
    emit(SOURCE_TICK, 1.0f);
    lastEmitTime = currentTime;
  }
}

void TimerSourceMod::applyIntent(const Intent& intent, float strength) {
  if (strength < 0.01) return;
  
  // Map Energy intent to interval (inverse relationship: higher energy = shorter interval)
  // Energy ranges 0-1, we want to map to interval range (0.01-10.0)
  // Use inverse exponential: high energy -> low interval (faster ticking)
  float energy = intent.getEnergy();
  if (energy > 0.01) {
    float mappedInterval = exponentialMap(1.0f - energy, intervalController, 0.5f);
    intervalController.updateIntent(mappedInterval, strength);
  }
}



} // ofxMarkSynth
