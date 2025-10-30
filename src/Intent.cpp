#include "Intent.hpp"

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

Intent Intent::weightedBlend(const std::vector<std::pair<IntentPtr, float>>& weightedIntents) {
  float totalWeight = 0.0f;
  float blendedEnergy = 0.0f;
  float blendedDensity = 0.0f;
  float blendedStructure = 0.0f;
  float blendedChaos = 0.0f;
  float blendedGranularity = 0.0f;
  
  for (const auto& pair : weightedIntents) {
    const auto& intent = pair.first;
    const auto& weight = pair.second;
    if (weight > 0.0f && intent) {
      totalWeight += weight;
      blendedEnergy += intent->getEnergy() * weight;
      blendedDensity += intent->getDensity() * weight;
      blendedStructure += intent->getStructure() * weight;
      blendedChaos += intent->getChaos() * weight;
      blendedGranularity += intent->getGranularity() * weight;
    }
  }
  
  if (totalWeight > 0.0f) {
    blendedEnergy /= totalWeight;
    blendedDensity /= totalWeight;
    blendedStructure /= totalWeight;
    blendedChaos /= totalWeight;
    blendedGranularity /= totalWeight;
  }
  
  return Intent("Blended", blendedEnergy, blendedDensity, blendedStructure, blendedChaos, blendedGranularity);
}

} // namespace ofxMarkSynth
