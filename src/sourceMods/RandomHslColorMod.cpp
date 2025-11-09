//
//  RandomHslColorMod.cpp
//  example_sandlines
//
//  Created by Steve Meyfroidt on 10/05/2025.
//

#include "RandomHslColorMod.hpp"
#include "IntentMapping.hpp"


namespace ofxMarkSynth {


RandomHslColorMod::RandomHslColorMod(Synth* synthPtr, const std::string& name, const ModConfig&& config)
: Mod { synthPtr, name, std::move(config) }
{
  sourceNameIdMap = {
    { "vec4", SOURCE_VEC4 }
  };
  
  sourceNameControllerPtrMap = {
    { colorsPerUpdateParameter.getName(), &colorsPerUpdateController },
    { minSaturationParameter.getName(), &minSaturationController },
    { maxSaturationParameter.getName(), &maxSaturationController },
    { minLightnessParameter.getName(), &minLightnessController },
    { maxLightnessParameter.getName(), &maxLightnessController },
    { minAlphaParameter.getName(), &minAlphaController },
    { maxAlphaParameter.getName(), &maxAlphaController }
  };
}

void RandomHslColorMod::initParameters() {
  parameters.add(colorsPerUpdateParameter);
  parameters.add(minSaturationParameter);
  parameters.add(maxSaturationParameter);
  parameters.add(minLightnessParameter);
  parameters.add(maxLightnessParameter);
  parameters.add(minAlphaParameter);
  parameters.add(maxAlphaParameter);
}

void RandomHslColorMod::update() {
  colorsPerUpdateController.update();
  minSaturationController.update();
  maxSaturationController.update();
  minLightnessController.update();
  maxLightnessController.update();
  minAlphaController.update();
  maxAlphaController.update();
  
  colorCount += colorsPerUpdateController.value;
  int colorsToCreate = std::floor(colorCount);
  colorCount -= colorsToCreate;
  if (colorsToCreate == 0) return;

  for (int i = 0; i < colorsToCreate; i++) {
    auto c = createRandomColor();
    emit(SOURCE_VEC4, glm::vec4 { c.r, c.g, c.b, c.a });
  }
}

const ofFloatColor RandomHslColorMod::createRandomColor() const {
  auto c = ofFloatColor::fromHsb(ofRandom(),
                                 ofRandom(minSaturationController.value, maxSaturationController.value),
                                 ofRandom(minLightnessController.value, maxLightnessController.value));
  c.a = ofRandom(minAlphaController.value, maxAlphaController.value);
  return c;
}

void RandomHslColorMod::applyIntent(const Intent& intent, float strength) {
  if (strength < 0.01) return;

  // TODO: fix these
//  colorsPerUpdateController.updateIntent(exponentialMap(intent.getDensity(), 0.5f, 10.0f, 2.0f), strength);
//  minSaturationController.updateIntent(linearMap(intent.getEnergy(), 0.3f, 0.8f), strength);
//  maxSaturationController.updateIntent(linearMap(intent.getEnergy(), 0.6f, 1.0f), strength);
//  minLightnessController.updateIntent(inverseMap(intent.getStructure(), 0.6f, 0.2f), strength);
//  maxLightnessController.updateIntent(inverseMap(intent.getStructure(), 1.0f, 0.7f), strength);
//  minAlphaController.updateIntent(linearMap(intent.getDensity(), 0.3f, 0.7f), strength);
//  maxAlphaController.updateIntent(linearMap(intent.getDensity(), 0.6f, 1.0f), strength);
}


} // ofxMarkSynth
