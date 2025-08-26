//
//  SomPaletteMod.cpp
//  example_audio_palette
//
//  Created by Steve Meyfroidt on 06/05/2025.
//

#include "SomPaletteMod.hpp"


namespace ofxMarkSynth {


SomPaletteMod::SomPaletteMod(const std::string& name, const ModConfig&& config)
: Mod { name, std::move(config) }
{
  somPalette.numIterations = iterationsParameter;
  somPalette.setVisible(false);
}

void SomPaletteMod::initParameters() {
  parameters.add(iterationsParameter);
}

void SomPaletteMod::update() {
  std::for_each(newVecs.cbegin(), newVecs.cend(), [this](const auto& v){
    std::array<double, 3> data { v.x, v.y, v.z };
    somPalette.addInstanceData(data);
  });
  newVecs.clear();
  somPalette.update();
  
  emit(SOURCE_RANDOM_VEC4, createRandomVec4(randomDistrib(randomGen)));
  emit(SOURCE_RANDOM_LIGHT_VEC4, createRandomLightVec4(randomDistrib(randomGen)));
  emit(SOURCE_RANDOM_DARK_VEC4, createRandomDarkVec4(randomDistrib(randomGen)));
  const ofFloatColor& darkestColor = somPalette.getColor(0);
  emit(SOURCE_DARKEST_VEC4, glm::vec4 { darkestColor.r, darkestColor.g, darkestColor.b, 1.0 });
}

glm::vec4 SomPaletteMod::createRandomVec4(int i) {
  ofFloatColor c = somPalette.getColor(i);
  return { c.r, c.g, c.b, c.a };
}

glm::vec4 SomPaletteMod::createRandomLightVec4(int i) {
  ofFloatColor c = somPalette.getColor((SomPalette::size - 1) - i/2);
  return { c.r, c.g, c.b, c.a };
}

glm::vec4 SomPaletteMod::createRandomDarkVec4(int i) {
  ofFloatColor c = somPalette.getColor(i/2);
  return { c.r, c.g, c.b, c.a };
}

void SomPaletteMod::draw() {
  somPalette.draw();
}

bool SomPaletteMod::keyPressed(int key) {
  return somPalette.keyPressed(key);
}

void SomPaletteMod::receive(int sinkId, const glm::vec3& v) {
  switch (sinkId) {
    case SINK_VEC3:
      newVecs.push_back(v);
      break;
    default:
      ofLogError() << "glm::vec3 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void SomPaletteMod::receive(int sinkId, const float& v) {
  switch (sinkId) {
    case SINK_AUDIO_TIMBRE_CHANGE:
      ofLogNotice() << "SomPaletteMod::receive audio timbre change; switching palette";
      somPalette.switchPalette();
      break;
    default:
      ofLogError() << "float receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

float SomPaletteMod::bidToReceive(int sinkId) {
  switch (sinkId) {
    case SINK_AUDIO_TIMBRE_CHANGE:
      if (somPalette.nextPaletteIsReady()) return 0.8;
    default:
      return 0.0;
  }
}
  


} // ofxMarkSynth
