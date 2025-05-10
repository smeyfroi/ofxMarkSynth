//
//  RandomHslColorMod.cpp
//  example_sandlines
//
//  Created by Steve Meyfroidt on 10/05/2025.
//

#include "RandomHslColorMod.hpp"


namespace ofxMarkSynth {


RandomHslColorMod::RandomHslColorMod(const std::string& name, const ModConfig&& config)
: Mod { name, std::move(config) }
{}

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
  colorCount += colorsPerUpdateParameter;
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
                                 ofRandom(minSaturationParameter, maxSaturationParameter),
                                 ofRandom(minLightnessParameter, maxLightnessParameter));
  c.a = ofRandom(minAlphaParameter, maxAlphaParameter);
  return c;
}


} // ofxMarkSynth
