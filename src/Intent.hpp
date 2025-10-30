#pragma once

#include "ofxGui.h"
#include <memory>
#include <vector>
#include <string>

namespace ofxMarkSynth {



class Intent;
using IntentPtr = std::shared_ptr<Intent>;



class Intent {
public:
  Intent(const std::string& name, 
         float energy, float density, float structure, float chaos);
  
  static IntentPtr createPreset(const std::string& name,
                                float energy, float density,
                                float structure, float chaos);
  
  void setEnergy(float v) { energyParameter = v; }
  void setDensity(float v) { densityParameter = v; }
  void setStructure(float v) { structureParameter = v; }
  void setChaos(float v) { chaosParameter = v; }
  
  std::string getName() const { return name; }
  float getEnergy() const { return energyParameter; }
  float getDensity() const { return densityParameter; }
  float getStructure() const { return structureParameter; }
  float getChaos() const { return chaosParameter; }
  
  ofParameterGroup& getParameterGroup() { return parameters; }
  
  static Intent weightedBlend(const std::vector<std::pair<IntentPtr, float>>& weightedIntents);
  
private:
  std::string name;
  ofParameterGroup parameters;
  ofParameter<float> energyParameter { "Energy", 0.5, 0.0, 1.0 };
  ofParameter<float> densityParameter { "Density", 0.5, 0.0, 1.0 };
  ofParameter<float> structureParameter { "Structure", 0.5, 0.0, 1.0 };
  ofParameter<float> chaosParameter { "Chaos", 0.5, 0.0, 1.0 };
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
