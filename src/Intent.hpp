#pragma once

#include "ofxGui.h"
#include <memory>
#include <vector>
#include <string>

namespace ofxMarkSynth {



class Intent;
using IntentPtr = std::shared_ptr<Intent>;



/*
 * Energy -> amount of motion, speed, activity, jitter
 * Density -> amount of elements, complexity, detail
 * Structure -> organization, patterns, repetition
 * Chaos -> randomness, unpredictability, noise
 * Granularity -> scale of features
 */
class Intent {
public:
  Intent(const std::string& name,
         float energy, float density, float structure, float chaos, float granularity);
  
  static IntentPtr createPreset(const std::string& name,
                                float energy, float density,
                                float structure, float chaos,
                                float granularity);
  
  void setEnergy(float v) { energyParameter = v; }
  void setDensity(float v) { densityParameter = v; }
  void setStructure(float v) { structureParameter = v; }
  void setChaos(float v) { chaosParameter = v; }
  void setGranularity(float v) { granularityParameter = v; }
  
  std::string getName() const { return name; }
  float getEnergy() const { return energyParameter; }
  float getDensity() const { return densityParameter; }
  float getStructure() const { return structureParameter; }
  float getChaos() const { return chaosParameter; }
  float getGranularity() const { return granularityParameter; }
  
  ofParameterGroup& getParameterGroup() { return parameters; }
  
  void setWeightedBlend(const std::vector<std::pair<IntentPtr, float>>& weightedIntents);
  
private:
  std::string name;
  ofParameterGroup parameters;
  ofParameter<float> energyParameter { "Energy", 0.5, 0.0, 1.0 };
  ofParameter<float> densityParameter { "Density", 0.5, 0.0, 1.0 };
  ofParameter<float> structureParameter { "Structure", 0.5, 0.0, 1.0 };
  ofParameter<float> chaosParameter { "Chaos", 0.5, 0.0, 1.0 };
  ofParameter<float> granularityParameter { "Granularity", 0.5, 0.0, 1.0 };
};

struct IntentActivation {
  IntentPtr intentPtr;
  float activation;
  float targetActivation;
  float transitionSpeed;
  
  IntentActivation(IntentPtr intentPtr_)
  : intentPtr(intentPtr_), activation(0.0), targetActivation(0.0), transitionSpeed(0.5) {}
};

using IntentActivations = std::vector<IntentActivation>;

} // namespace ofxMarkSynth
