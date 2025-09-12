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

// u = 0.7071 R + 0.7071 G
// v = 0.5774 R + 0.5774 G - 1.1547 B
ofFloatPixels rgbToRG_Simple(const ofFloatPixels& in) {
  const int w = in.getWidth();
  const int h = in.getHeight();
  
  ofFloatPixels result;
  result.allocate(w, h, 2);
  
  const float* src = in.getData();
  float* dst = result.getData();
  const size_t n = static_cast<size_t>(w) * static_cast<size_t>(h);
  
  constexpr float uR = 0.70710678f, uG = 0.70710678f, uB = 0.0f;
  constexpr float vR = 0.57735027f, vG = 0.57735027f, vB = -1.15470054f;
  
  for (size_t i = 0; i < n; ++i) {
    float R = src[3*i + 0];
    float G = src[3*i + 1];
    float B = src[3*i + 2];
    
    float u = uR*R + uG*G + uB*B;
    float v = vR*R + vG*G + vB*B;
    
    dst[2*i + 0] = u;
    dst[2*i + 1] = v;
  }
  return result;
}

void SomPaletteMod::update() {
  std::for_each(newVecs.cbegin(), newVecs.cend(), [this](const auto& v){
    std::array<double, 3> data { v.x, v.y, v.z };
    somPalette.addInstanceData(data);
  });
  newVecs.clear();
  somPalette.update();
  
  emit(SOURCE_RANDOM_VEC4, createVec4(randomDistrib(randomGen)));
  emit(SOURCE_RANDOM_LIGHT_VEC4, createRandomLightVec4(randomDistrib(randomGen)));
  emit(SOURCE_RANDOM_DARK_VEC4, createRandomDarkVec4(randomDistrib(randomGen)));
  emit(SOURCE_DARKEST_VEC4, createVec4(0));
  emit(SOURCE_FIELD, rgbToRG_Simple(somPalette.getPixelsRef())); // we make a copy
}

glm::vec4 SomPaletteMod::createVec4(int i) {
  ofFloatColor c = somPalette.getColor(i);
  return { c.r, c.g, c.b, 1.0 };
}

glm::vec4 SomPaletteMod::createRandomLightVec4(int i) {
  return createVec4((SomPalette::size - 1) - i/2);
}

glm::vec4 SomPaletteMod::createRandomDarkVec4(int i) {
  return createVec4(i/2);
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
