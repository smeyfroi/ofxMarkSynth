#pragma once

#include "ofxGui.h"
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace ofxMarkSynth {



class Intent;
using IntentPtr = std::shared_ptr<Intent>;



/*
 • Energy → speed, motion, intensity, magnitude, size, activity level
 • Density → quantity, opacity, detail level, connection strength
 • Structure → organization, alignment, brightness, pattern regularity
 • Chaos → randomness, variance, noise, unpredictability, disorder
 • Granularity → scale, resolution, feature size, clustering; high is larger
 */
class Intent {
public:
  // Ordered list of label -> [-3..+3]. Order is preserved from config JSON.
  using UiImpact = std::vector<std::pair<std::string, int>>;

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
  const ofParameterGroup& getParameterGroup() const { return parameters; }

  void setWeightedBlend(const std::vector<std::pair<IntentPtr, float>>& weightedIntents);

  // Optional config-driven UI metadata (safe to omit in configs).
  void setUiImpact(const UiImpact& impact);
  const std::optional<UiImpact>& getUiImpact() const { return uiImpact; }

  void setUiNotes(const std::string& notes);
  const std::optional<std::string>& getUiNotes() const { return uiNotes; }

private:
  std::string name;
  ofParameterGroup parameters;
  ofParameter<float> energyParameter { "Energy", 0.5, 0.0, 1.0 };
  ofParameter<float> densityParameter { "Density", 0.5, 0.0, 1.0 };
  ofParameter<float> structureParameter { "Structure", 0.5, 0.0, 1.0 };
  ofParameter<float> chaosParameter { "Chaos", 0.5, 0.0, 1.0 };
  ofParameter<float> granularityParameter { "Granularity", 0.5, 0.0, 1.0 };

  std::optional<UiImpact> uiImpact;
  std::optional<std::string> uiNotes;
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
