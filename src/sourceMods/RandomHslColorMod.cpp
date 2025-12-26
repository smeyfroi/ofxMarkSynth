//
//  RandomHslColorMod.cpp
//  example_sandlines
//
//  Created by Steve Meyfroidt on 10/05/2025.
//

#include "RandomHslColorMod.hpp"
#include "IntentMapping.hpp"
#include "IntentMapper.hpp"


namespace ofxMarkSynth {


RandomHslColorMod::RandomHslColorMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config)
: colorCount { 0.0f },
Mod { synthPtr, name, std::move(config) }
{
  sourceNameIdMap = {
    { "Vec4", SOURCE_VEC4 }
  };
  
  registerControllerForSource(colorsPerUpdateParameter, colorsPerUpdateController);
  registerControllerForSource(hueCenterParameter, hueCenterController);
  registerControllerForSource(hueWidthParameter, hueWidthController);
  registerControllerForSource(minSaturationParameter, minSaturationController);
  registerControllerForSource(maxSaturationParameter, maxSaturationController);
  registerControllerForSource(minBrightnessParameter, minBrightnessController);
  registerControllerForSource(maxBrightnessParameter, maxBrightnessController);
  registerControllerForSource(minAlphaParameter, minAlphaController);
  registerControllerForSource(maxAlphaParameter, maxAlphaController);
  
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
  IntentMap im(intent);

  im.D().exp(colorsPerUpdateController, strength, 2.0f);

  // Hue: E -> center (cool to warm), C -> width
  float targetCenter = ofLerp(0.6f, 0.08f, im.E().get());
  float targetWidth  = ofLerp(0.08f, 1.0f, im.C().get());
  hueCenterController.updateIntent(targetCenter, strength, "E -> hue center");
  hueWidthController.updateIntent(targetWidth, strength, "C -> hue width");

  im.E().lin(minSaturationController, strength, 0.2f, 0.8f);
  im.E().lin(maxSaturationController, strength, 0.6f, 1.0f);

  im.S().inv().lin(minBrightnessController, strength, 0.1f, 0.4f);
  im.S().lin(maxBrightnessController, strength, 0.6f, 1.0f);

  im.D().lin(minAlphaController, strength, 0.2f, 0.8f);
  im.D().lin(maxAlphaController, strength, 0.6f, 1.0f);
}


} // ofxMarkSynth
