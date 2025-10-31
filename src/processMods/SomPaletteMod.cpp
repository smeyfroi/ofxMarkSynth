//
//  SomPaletteMod.cpp
//  example_audio_palette
//
//  Created by Steve Meyfroidt on 06/05/2025.
//

#include "SomPaletteMod.hpp"
#include "IntentMapping.hpp"


namespace ofxMarkSynth {


SomPaletteMod::SomPaletteMod(Synth* synthPtr, const std::string& name, const ModConfig&& config)
: Mod { synthPtr, name, std::move(config) }
{
  somPalette.numIterations = iterationsParameter;
//  somPalette.numIterations = iterationsController.value;
  somPalette.setVisible(false);
  
  sinkNameIdMap = {
    { "vec3", SINK_VEC3 },
    { "switchPalette", SINK_SWITCH_PALETTE }
  };
  sourceNameIdMap = {
    { "random", SOURCE_RANDOM },
    { "randomLight", SOURCE_RANDOM_LIGHT },
    { "randomDark", SOURCE_RANDOM_DARK },
    { "darkest", SOURCE_DARKEST },
    { "field", SOURCE_FIELD }
  };
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
//  learningRateController.update();
//  iterationsController.update();
  if (newVecs.empty()) return;
  
  std::for_each(newVecs.cbegin(), newVecs.cend(), [this](const auto& v){
    std::array<double, 3> data { v.x, v.y, v.z };
    somPalette.addInstanceData(data);
  });
  newVecs.clear();
  somPalette.update();
  
  emit(SOURCE_RANDOM, createVec4(randomDistrib(randomGen)));
  emit(SOURCE_RANDOM_LIGHT, createRandomLightVec4(randomDistrib(randomGen)));
  emit(SOURCE_RANDOM_DARK, createRandomDarkVec4(randomDistrib(randomGen)));
  emit(SOURCE_DARKEST, createVec4(0));
  
  // convert RGB -> RG (float2), upload into FBO texture, emit FBO
  const ofPixels& pixelsRef = somPalette.getPixelsRef();
  if (pixelsRef.getWidth() == 0 || pixelsRef.getHeight() == 0) return;
  ofFloatPixels converted = rgbToRG_Opponent(pixelsRef);
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
    case SINK_SWITCH_PALETTE:
      if (somPalette.nextPaletteIsReady() && v > 0.5f) { // TODO: Convert to a parameter *****************
        ofLogNotice() << "SomPaletteMod switching palette";
        somPalette.switchPalette();
      }
      break;
    default:
      ofLogError() << "float receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void SomPaletteMod::applyIntent(const Intent& intent, float strength) {
  if (strength < 0.01) return;
//  float targetIterations = linearMap(intent.getStructure() * intent.getDensity(), 1000.0f, 20000.0f);
//  iterationsParameter = static_cast<int>(ofLerp(iterationsParameter.get(), targetIterations, strength * 0.1f));
}
  


} // ofxMarkSynth
