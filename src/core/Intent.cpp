#include "core/Intent.hpp"

namespace ofxMarkSynth {

Intent::Intent(const std::string& name_,
               float energy, float density, float structure, float chaos, float granularity)
: name(name_)
{
  parameters.setName(name);
  parameters.add(energyParameter.set("Energy", energy, 0.0, 1.0));
  parameters.add(densityParameter.set("Density", density, 0.0, 1.0));
  parameters.add(structureParameter.set("Structure", structure, 0.0, 1.0));
  parameters.add(chaosParameter.set("Chaos", chaos, 0.0, 1.0));
  parameters.add(granularityParameter.set("Granularity", granularity, 0.0, 1.0));
}

IntentPtr Intent::createPreset(const std::string& name,
                               float energy, float density,
                               float structure, float chaos,
                               float granularity) {
  return std::make_shared<Intent>(name, energy, density, structure, chaos, granularity);
}

void Intent::setWeightedBlend(const std::vector<std::pair<IntentPtr, float>>& weightedIntents) {
  // Calculate total weight for normalization
  float totalWeight = 0.0f;
  for (const auto& pair : weightedIntents) {
    float weight = pair.second;
    if (weight > 0.0001f && pair.first) {
      totalWeight += weight;
    }
  }
  
  // If no active intents, set to zero (no Intent influence)
  if (totalWeight < 0.0001f) {
    energyParameter = 0.0f;
    densityParameter = 0.0f;
    structureParameter = 0.0f;
    chaosParameter = 0.0f;
    granularityParameter = 0.0f;
    return;
  }
  
  float blendedEnergy = 0.0f;
  float blendedDensity = 0.0f;
  float blendedStructure = 0.0f;
  float blendedChaos = 0.0f;
  float blendedGranularity = 0.0f;

  for (const auto& pair : weightedIntents) {
    const auto& intent = pair.first;
    float weight = pair.second;

    if (weight > 0.0001f && intent) {
      // Normalize weight so all weights sum to 1.0
      float normalizedWeight = weight / totalWeight;
      blendedEnergy += intent->getEnergy() * normalizedWeight;
      blendedDensity += intent->getDensity() * normalizedWeight;
      blendedStructure += intent->getStructure() * normalizedWeight;
      blendedChaos += intent->getChaos() * normalizedWeight;
      blendedGranularity += intent->getGranularity() * normalizedWeight;
    }
  }

  energyParameter = blendedEnergy;
  densityParameter = blendedDensity;
  structureParameter = blendedStructure;
  chaosParameter = blendedChaos;
  granularityParameter = blendedGranularity;
}



} // namespace ofxMarkSynth
