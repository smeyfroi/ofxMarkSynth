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

ofFloatPixels rgbToRG_Opponent(const ofFloatPixels& in) {
  const int w = in.getWidth();
  const int h = in.getHeight();

  ofFloatPixels out;
  out.allocate(w, h, 2);

  const float* src = in.getData();
  float* dst = out.getData();
  const size_t n = static_cast<size_t>(w) * static_cast<size_t>(h);

  // Orthonormal opponent axes:
  // e1 = ( 1, -1,  0) / sqrt(2)  -> red-green
  // e2 = ( 1,  1, -2) / sqrt(6)  -> blue-yellow
  constexpr float invSqrt2 = 0.7071067811865476f;
  constexpr float invSqrt6 = 0.4082482904638631f;

  for (size_t i = 0; i < n; ++i) {
    float R = src[3*i + 0];
    float G = src[3*i + 1];
    float B = src[3*i + 2];

    // center around neutral gray to make outputs zero-mean
    float r = R - 0.5f;
    float g = G - 0.5f;
    float b = B - 0.5f;

    float u = (r - g) * invSqrt2;              // red-green
    float v = (r + g - 2.0f*b) * invSqrt6;     // blue-yellow

    dst[2*i + 0] = u;
    dst[2*i + 1] = v;
  }
  return out;
}

void SomPaletteMod::ensureFieldFbo(int w, int h) {
  if (fieldFbo.isAllocated()
      && fieldFbo.getWidth() == w && fieldFbo.getHeight() == h) {
    return;
  }
  ofFbo::Settings s;
  s.width = w;
  s.height = h;
  s.internalformat = GL_RG16F;
  s.useDepth = false;
  s.useStencil = false;
  s.numColorbuffers = 1;
  s.textureTarget = GL_TEXTURE_2D;
  fieldFbo.allocate(s);
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
  
  // convert RGB -> RG (float2), upload into FBO texture, emit FBO
  ofFloatPixels converted = rgbToRG_Opponent(somPalette.getPixelsRef());
  ensureFieldFbo(converted.getWidth(), converted.getHeight());
  fieldFbo.getTexture().loadData(converted);
  emit(SOURCE_FIELD, fieldFbo);
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
