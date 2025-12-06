//
//  SomPaletteMod.cpp
//  example_audio_palette
//
//  Created by Steve Meyfroidt on 06/05/2025.
//

#include "SomPaletteMod.hpp"
#include "IntentMapping.hpp"
#include "Synth.hpp"


namespace ofxMarkSynth {


SomPaletteMod::SomPaletteMod(Synth* synthPtr, const std::string& name, ModConfig config)
: Mod { synthPtr, name, std::move(config) }
{
  somPalette.numIterations = iterationsParameter;
  //  somPalette.numIterations = iterationsController.value;
  somPalette.setVisible(false);
  
  sinkNameIdMap = {
    { "Sample", SINK_VEC3 },
    { "SwitchPalette", SINK_SWITCH_PALETTE }
  };
  sourceNameIdMap = {
    { "Random", SOURCE_RANDOM },
    { "RandomLight", SOURCE_RANDOM_LIGHT },
    { "RandomDark", SOURCE_RANDOM_DARK },
    { "Darkest", SOURCE_DARKEST },
    { "Lightest", SOURCE_LIGHTEST },
    { "FieldTexture", SOURCE_FIELD }
  };
}

void SomPaletteMod::doneModLoad() {
  if (!synthPtr) return;
  auto self = std::static_pointer_cast<SomPaletteMod>(shared_from_this());
  std::string baseName = getName();
  
  synthPtr->addLiveTexturePtrFn(baseName + ": Active",
                                [weakSelf = std::weak_ptr<SomPaletteMod>(self)]() -> const ofTexture* {
    if (auto locked = weakSelf.lock()) {
      return locked->getActivePaletteTexturePtr();
    }
    return nullptr;
  });
  
  synthPtr->addLiveTexturePtrFn(baseName + ": Next",
                                [weakSelf = std::weak_ptr<SomPaletteMod>(self)]() -> const ofTexture* {
    if (auto locked = weakSelf.lock()) {
      return locked->getNextPaletteTexturePtr();
    }
    return nullptr;
  });
}


SomPaletteMod::~SomPaletteMod() {
  iterationsParameter.removeListener(this, &SomPaletteMod::onIterationsParameterChanged);
}

void SomPaletteMod::onIterationsParameterChanged(float& value) {
  somPalette.setNumIterations(iterationsParameter);
}

void SomPaletteMod::initParameters() {
  parameters.add(iterationsParameter);
  parameters.add(agencyFactorParameter);
  iterationsParameter.addListener(this, &SomPaletteMod::onIterationsParameterChanged);
}

float SomPaletteMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

// Texture values have a range approximately -0.7 to 0.7
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

void SomPaletteMod::ensureFieldTexture(int w, int h) {
  if (fieldTexture.isAllocated()
      && fieldTexture.getWidth() == w && fieldTexture.getHeight() == h) {
    return;
  }
  ofTextureData texData;
  texData.width = w;
  texData.height = h;
  texData.textureTarget = GL_TEXTURE_2D;
  texData.glInternalFormat = GL_RG16F;
  texData.bFlipTexture = false;
  texData.wrapModeHorizontal = GL_REPEAT;
  texData.wrapModeVertical = GL_REPEAT;
  
  fieldTexture.allocate(texData);
}

void SomPaletteMod::update() {
  syncControllerAgencies();
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
  emit(SOURCE_LIGHTEST, createVec4(SomPalette::size - 1));
  
  // convert RGB -> RG (float2), upload into FBO texture, emit FBO
  const ofPixels& pixelsRef = somPalette.getPixelsRef();
  if (pixelsRef.getWidth() == 0 || pixelsRef.getHeight() == 0) return;
  ofFloatPixels converted = rgbToRG_Opponent(pixelsRef);
  ensureFieldTexture(converted.getWidth(), converted.getHeight());
  fieldTexture.loadData(converted);
  emit(SOURCE_FIELD, fieldTexture);
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
      ofLogError("SomPaletteMod") << "glm::vec3 receive for unknown sinkId " << sinkId;
  }
}

void SomPaletteMod::receive(int sinkId, const float& v) {
  switch (sinkId) {
    case SINK_SWITCH_PALETTE:
      if (somPalette.nextPaletteIsReady() && v > 0.5f) { // TODO: Convert to a parameter *****************
        ofLogNotice("SomPaletteMod") << "SomPaletteMod switching palette";
        somPalette.switchPalette();
      }
      break;
    default:
      ofLogError("SomPaletteMod") << "Float receive for unknown sinkId " << sinkId;
  }
}

void SomPaletteMod::applyIntent(const Intent& intent, float strength) {
  //  float targetIterations = linearMap(intent.getStructure() * intent.getDensity(), 1000.0f, 20000.0f);
  //  iterationsParameter = static_cast<int>(ofLerp(iterationsParameter.get(), targetIterations, strength * 0.1f));
}

const ofTexture* SomPaletteMod::getActivePaletteTexturePtr() const {
  return somPalette.getActiveTexturePtr();
}

const ofTexture* SomPaletteMod::getNextPaletteTexturePtr() const {
  return somPalette.getNextTexturePtr();
}



} // ofxMarkSynth
