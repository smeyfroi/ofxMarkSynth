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

  sinkNameIdMap = {
    { intervalParameter.getName(), SINK_INTERVAL },
    { enabledParameter.getName(), SINK_ENABLED },
    { oneShotParameter.getName(), SINK_ONE_SHOT },
    { timeToNextParameter.getName(), SINK_TIME_TO_NEXT },
    { "Start", SINK_START },
    { "Stop", SINK_STOP },
    { "Reset", SINK_RESET },
    { "Trigger Now", SINK_TRIGGER_NOW }
  };
  
  sourceNameControllerPtrMap = {
    { intervalParameter.getName(), &intervalController }
  };
  
  float now = ofGetElapsedTimef();
  nextFireTime = now + std::max(intervalController.value, MIN_INTERVAL);
}

void TimerSourceMod::initParameters() {
  parameters.add(intervalParameter);
  parameters.add(enabledParameter);
  parameters.add(oneShotParameter);
  parameters.add(timeToNextParameter);
}

void TimerSourceMod::update() {
  intervalController.update();

  // Consume Time To Next if set via GUI/automation
  if (timeToNextParameter > 0.0f) {
    float now = ofGetElapsedTimef();
    nextFireTime = now + std::max(timeToNextParameter.get(), MIN_INTERVAL);
    timeToNextParameter.set(0.0f);
  }
  
  // If previously fired in one-shot and user switched One Shot off, rearm and re-enable
  if (!enabledParameter && hasFired && !oneShotParameter) {
    hasFired = false;
    enabledParameter.set(true);
    float now = ofGetElapsedTimef();
    nextFireTime = now + std::max(intervalController.value, MIN_INTERVAL);
  }
  
  if (!enabledParameter) {
    return;
  }
  
  float now = ofGetElapsedTimef();
  if (now >= nextFireTime) {
    emit(SOURCE_TICK, 1.0f);

    if (oneShotParameter) {
      enabledParameter.set(false);
      hasFired = true;
      // Do not reschedule when one-shot finishes
    } else {
      nextFireTime = now + std::max(intervalController.value, MIN_INTERVAL);
    }
  }
}

void TimerSourceMod::receive(int sinkId, const float& value) {
  switch (sinkId) {
    case SINK_INTERVAL:
      intervalController.updateAuto(value, getAgency());
      break;
    case SINK_ENABLED:
      enabledParameter.set(value > 0.5f);
      break;
    case SINK_ONE_SHOT: {
      bool wasOneShot = oneShotParameter.get();
      oneShotParameter.set(value > 0.5f);
      if (wasOneShot && !oneShotParameter && hasFired && !enabledParameter) {
        hasFired = false;
        enabledParameter.set(true);
        float now = ofGetElapsedTimef();
        nextFireTime = now + std::max(intervalController.value, MIN_INTERVAL);
      }
      break;
    }
    case SINK_TIME_TO_NEXT: {
      float now = ofGetElapsedTimef();
      nextFireTime = now + std::max(value, MIN_INTERVAL);
      break;
    }
    case SINK_START: {
      bool wasDisabled = !enabledParameter.get();
      enabledParameter.set(true);
      if (oneShotParameter && (hasFired || wasDisabled)) {
        hasFired = false;
        float now = ofGetElapsedTimef();
        nextFireTime = now + std::max(intervalController.value, MIN_INTERVAL);
      }
      break;
    }
    case SINK_STOP:
      enabledParameter.set(false);
      break;
    case SINK_RESET: {
      hasFired = false;
      float now = ofGetElapsedTimef();
      nextFireTime = now + std::max(intervalController.value, MIN_INTERVAL);
      break;
    }
    case SINK_TRIGGER_NOW: {
      if (enabledParameter) {
        float now = ofGetElapsedTimef();
        emit(SOURCE_TICK, 1.0f);
        if (oneShotParameter) {
          enabledParameter.set(false);
          hasFired = true;
        } else {
          nextFireTime = now + std::max(intervalController.value, MIN_INTERVAL);
        }
      }
      break;
    }
    default:
      ofLogError("TimerSourceMod") << "Float receive for unknown sinkId " << sinkId;
  }
}

void TimerSourceMod::applyIntent(const Intent& intent, float strength) {
  if (strength < 0.01) return;
  
  // Map Energy intent to interval (inverse relationship: higher energy = shorter interval)
  float energy = intent.getEnergy();
  if (energy > 0.01f) {
    float mappedInterval = exponentialMap(1.0f - energy, intervalController, 0.5f);
    intervalController.updateIntent(mappedInterval, strength);
  }
}



} // ofxMarkSynth
