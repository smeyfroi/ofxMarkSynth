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
: colorCount { 0.0f },
Mod { synthPtr, name, std::move(config) }
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
    { minBrightnessParameter.getName(), &minBrightnessController },
    { maxBrightnessParameter.getName(), &maxBrightnessController },
    { minAlphaParameter.getName(), &minAlphaController },
    { maxAlphaParameter.getName(), &maxAlphaController }
  };
  
  sinkNameIdMap = {
    { "ColorsPerUpdate", SINK_COLORS_PER_UPDATE },
    { "HueCenter", SINK_HUE_CENTER },
    { "HueWidth", SINK_HUE_WIDTH },
    { "MinSaturation", SINK_MIN_SATURATION },
    { "MaxSaturation", SINK_MAX_SATURATION },
    { "MinBrightness", SINK_MIN_BRIGHTNESS },
    { "MaxBrightness", SINK_MAX_BRIGHTNESS },
    { "MinAlpha", SINK_MIN_ALPHA },
    { "MaxAlpha", SINK_MAX_ALPHA }
  };
}

void RandomHslColorMod::initParameters() {
  parameters.add(colorsPerUpdateParameter);
  parameters.add(hueCenterParameter);
  parameters.add(hueWidthParameter);
  parameters.add(minSaturationParameter);
  parameters.add(maxSaturationParameter);
  parameters.add(minBrightnessParameter);
  parameters.add(maxBrightnessParameter);
  parameters.add(minAlphaParameter);
  parameters.add(maxAlphaParameter);
  parameters.add(agencyFactorParameter);
}

float RandomHslColorMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

void RandomHslColorMod::update() {
  syncControllerAgencies();
  colorsPerUpdateController.update();
  hueCenterController.update();
  hueWidthController.update();
  minSaturationController.update();
  maxSaturationController.update();
  minBrightnessController.update();
  maxBrightnessController.update();
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
    ofRandom(minBrightnessController.value, maxBrightnessController.value)
  );
  c.a = ofRandom(minAlphaController.value, maxAlphaController.value);
  return c;
}

void RandomHslColorMod::receive(int sinkId, const float& value) {
  float agency = getAgency();
  switch (sinkId) {
    case SINK_COLORS_PER_UPDATE:
      colorsPerUpdateController.updateAuto(value, agency);
      break;
    case SINK_HUE_CENTER:
      hueCenterController.updateAuto(value, agency);
      break;
    case SINK_HUE_WIDTH:
      hueWidthController.updateAuto(value, agency);
      break;
    case SINK_MIN_SATURATION:
      minSaturationController.updateAuto(value, agency);
      break;
    case SINK_MAX_SATURATION:
      maxSaturationController.updateAuto(value, agency);
      break;
    case SINK_MIN_BRIGHTNESS:
      minBrightnessController.updateAuto(value, agency);
      break;
    case SINK_MAX_BRIGHTNESS:
      maxBrightnessController.updateAuto(value, agency);
      break;
    case SINK_MIN_ALPHA:
      minAlphaController.updateAuto(value, agency);
      break;
    case SINK_MAX_ALPHA:
      maxAlphaController.updateAuto(value, agency);
      break;
  }
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

  // Structure → brightness contrast (higher structure = wider brightness range)
  minBrightnessController.updateIntent(inverseMap(intent.getStructure(), 0.4f, 0.1f), strength);
  maxBrightnessController.updateIntent(linearMap(intent.getStructure(), 0.6f, 1.0f), strength);

  // Density → alpha range (more density = more opaque)
  minAlphaController.updateIntent(linearMap(intent.getDensity(), 0.2f, 0.8f), strength);
  maxAlphaController.updateIntent(linearMap(intent.getDensity(), 0.6f, 1.0f), strength);
}


} // ofxMarkSynth
