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
  somPalette.setVisible(true);
}

void SomPaletteMod::initParameters() {
}

void SomPaletteMod::update() {
  std::for_each(newVecs.cbegin(), newVecs.cend(), [this](const auto& v){
    std::array<double, 3> data { v.x, v.y, v.z };
    somPalette.addInstanceData(data);
  });
  newVecs.clear();
  somPalette.update();
  
  emit(SOURCE_RANDOM_VEC4, createRandomVec4(randomDistrib(randomGen)));
}

glm::vec4 SomPaletteMod::createRandomVec4(int i) {
  ofFloatColor c = somPalette.getColor(i);
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


} // ofxMarkSynth
