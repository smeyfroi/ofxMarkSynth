//
//  RandomHslColorMod.cpp
//  example_sandlines
//
//  Created by Steve Meyfroidt on 10/05/2025.
//

#include "RandomHslColorMod.hpp"
#include "IntentMapping.hpp"


namespace ofxMarkSynth {


RandomHslColorMod::RandomHslColorMod(Synth* synthPtr, const std::string& name, ModConfig config)
: Mod { synthPtr, name, std::move(config) }
{
  sourceNameIdMap = {
    { "Vec4", SOURCE_VEC4 }
  };
  
  sourceNameControllerPtrMap = {
    { colorsPerUpdateParameter.getName(), &colorsPerUpdateController },
    { hueCenterParameter.getName(), &hueCenterController },
    { hueWidthParameter.getName(), &hueWidthController },
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
  parameters.add(hueCenterParameter);
  parameters.add(hueWidthParameter);
  parameters.add(minSaturationParameter);
  parameters.add(maxSaturationParameter);
  parameters.add(minLightnessParameter);
  parameters.add(maxLightnessParameter);
  parameters.add(minAlphaParameter);
  parameters.add(maxAlphaParameter);
}

void RandomHslColorMod::update() {
  syncControllerAgencies();
  colorsPerUpdateController.update();
  hueCenterController.update();
  hueWidthController.update();
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
  float hue = randomHueFromCenterWidth(hueCenterController.value, hueWidthController.value);
  auto c = ofFloatColor::fromHsb(
    hue,
    ofRandom(minSaturationController.value, maxSaturationController.value),
    ofRandom(minLightnessController.value, maxLightnessController.value)
  );
  c.a = ofRandom(minAlphaController.value, maxAlphaController.value);
  return c;
}

float RandomHslColorMod::randomHueFromCenterWidth(float center, float width) const {
  auto wrap01 = [](float v){ v = fmodf(v, 1.0f); return v < 0.0f ? v + 1.0f : v; };
  center = wrap01(center);
  width  = ofClamp(width, 0.0f, 1.0f);
  float half = 0.5f * width;
  return wrap01(center + ofRandom(-half, half));
}

void RandomHslColorMod::applyIntent(const Intent& intent, float strength) {

  // Density → number of colors per update (non-linear)
  colorsPerUpdateController.updateIntent(exponentialMap(intent.getDensity(), colorsPerUpdateController, 2.0f), strength);

  // Intent → Hue center/width (Energy shifts center; Chaos widens)
  float targetCenter = ofLerp(0.6f, 0.08f, intent.getEnergy());
  float targetWidth  = ofLerp(0.08f, 1.0f, intent.getChaos());
  hueCenterController.updateIntent(targetCenter, strength);
  hueWidthController.updateIntent(targetWidth, strength);

  // Energy → saturation range (higher energy = more saturated)
  minSaturationController.updateIntent(linearMap(intent.getEnergy(), 0.2f, 0.8f), strength);
  maxSaturationController.updateIntent(linearMap(intent.getEnergy(), 0.6f, 1.0f), strength);

  // Structure → lightness contrast (higher structure = wider lightness range)
  minLightnessController.updateIntent(inverseMap(intent.getStructure(), 0.4f, 0.1f), strength);
  maxLightnessController.updateIntent(linearMap(intent.getStructure(), 0.6f, 1.0f), strength);

  // Density → alpha range (more density = more opaque)
  minAlphaController.updateIntent(linearMap(intent.getDensity(), 0.2f, 0.8f), strength);
  maxAlphaController.updateIntent(linearMap(intent.getDensity(), 0.6f, 1.0f), strength);
}


} // ofxMarkSynth
