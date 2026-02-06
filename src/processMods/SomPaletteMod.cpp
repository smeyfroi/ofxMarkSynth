//  SomPaletteMod.cpp
//  example_audio_palette
//
//  Created by Steve Meyfroidt on 06/05/2025.
//

#include "SomPaletteMod.hpp"

#include "core/IntentMapping.hpp"
#include "core/Synth.hpp"

#include <algorithm>
#include <cmath>

namespace ofxMarkSynth {

namespace {

constexpr float ASSUMED_FPS = 30.0f;

int secsToFrames(float secs) {
  return std::max(1, static_cast<int>(std::round(secs * ASSUMED_FPS)));
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
    float R = src[3 * i + 0];
    float G = src[3 * i + 1];
    float B = src[3 * i + 2];

    // center around neutral gray to make outputs zero-mean
    float r = R - 0.5f;
    float g = G - 0.5f;
    float b = B - 0.5f;

    float u = (r - g) * invSqrt2; // red-green
    float v = (r + g - 2.0f * b) * invSqrt6; // blue-yellow

    dst[2 * i + 0] = u;
    dst[2 * i + 1] = v;
  }
  return out;
}

} // namespace

SomPaletteMod::SomPaletteMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config)
: Mod { synthPtr, name, std::move(config) }
{
  somPalette.setNumIterations(static_cast<int>(iterationsParameter.get()));

  windowFrames = secsToFrames(windowSecsParameter.get());
  somPalette.setWindowFrames(windowFrames);

  somPalette.setVisible(false);
  somPalette.setColorizerGains(colorizerGrayGainParameter, colorizerChromaGainParameter);

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
  auto synth = getSynth();
  if (!synth) return;
  auto self = std::static_pointer_cast<SomPaletteMod>(shared_from_this());
  std::string baseName = getName();

  synth->addLiveTexturePtrFn(baseName + ": Active",
                             [weakSelf = std::weak_ptr<SomPaletteMod>(self)]() -> const ofTexture* {
    if (auto locked = weakSelf.lock()) {
      return locked->getActivePaletteTexturePtr();
    }
    return nullptr;
  });

  synth->addLiveTexturePtrFn(baseName + ": Next",
                             [weakSelf = std::weak_ptr<SomPaletteMod>(self)]() -> const ofTexture* {
    if (auto locked = weakSelf.lock()) {
      return locked->getNextPaletteTexturePtr();
    }
    return nullptr;
  });
}

SomPaletteMod::~SomPaletteMod() {
  iterationsParameter.removeListener(this, &SomPaletteMod::onIterationsParameterChanged);
  windowSecsParameter.removeListener(this, &SomPaletteMod::onWindowSecsParameterChanged);
  colorizerGrayGainParameter.removeListener(this, &SomPaletteMod::onColorizerParameterChanged);
  colorizerChromaGainParameter.removeListener(this, &SomPaletteMod::onColorizerParameterChanged);
}

const ofTexture* SomPaletteMod::getActivePaletteTexturePtr() const {
  return somPalette.getActiveTexturePtr();
}

const ofTexture* SomPaletteMod::getNextPaletteTexturePtr() const {
  return somPalette.getNextTexturePtr();
}

void SomPaletteMod::onIterationsParameterChanged(float& value) {
  somPalette.setNumIterations(static_cast<int>(iterationsParameter.get()));
}

void SomPaletteMod::onWindowSecsParameterChanged(float& value) {
  windowFrames = secsToFrames(windowSecsParameter.get());
  somPalette.setWindowFrames(windowFrames);
  while (static_cast<int>(featureHistory.size()) > windowFrames) {
    featureHistory.pop_front();
  }
}

void SomPaletteMod::onColorizerParameterChanged(float& value) {
  somPalette.setColorizerGains(colorizerGrayGainParameter, colorizerChromaGainParameter);
}

void SomPaletteMod::initParameters() {
  parameters.add(iterationsParameter);
  parameters.add(windowSecsParameter);
  parameters.add(trainingStepsPerFrameParameter);
  parameters.add(agencyFactorParameter);
  parameters.add(colorizerGrayGainParameter);
  parameters.add(colorizerChromaGainParameter);

  iterationsParameter.addListener(this, &SomPaletteMod::onIterationsParameterChanged);
  windowSecsParameter.addListener(this, &SomPaletteMod::onWindowSecsParameterChanged);
  colorizerGrayGainParameter.addListener(this, &SomPaletteMod::onColorizerParameterChanged);
  colorizerChromaGainParameter.addListener(this, &SomPaletteMod::onColorizerParameterChanged);
}

float SomPaletteMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

void SomPaletteMod::ensureFieldTexture(int w, int h) {
  if (fieldTexture.isAllocated() && fieldTexture.getWidth() == w && fieldTexture.getHeight() == h) {
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

  // Expect one incoming feature per frame, but be robust to bursts.
  bool hasNewFeature = !newVecs.empty();
  glm::vec3 newestFeature;
  if (hasNewFeature) {
    newestFeature = newVecs.back();
    newVecs.clear();

    featureHistory.push_back(newestFeature);
    while (static_cast<int>(featureHistory.size()) > windowFrames) {
      featureHistory.pop_front();
    }
  }

  // Train multiple steps per frame by sampling from the recent history.
  const int steps = std::max(1, trainingStepsPerFrameParameter.get());
  if (!featureHistory.empty()) {
    std::uniform_int_distribution<size_t> dist(0, featureHistory.size() - 1);

    for (int i = 0; i < steps; ++i) {
      const glm::vec3 v = (hasNewFeature && i == 0) ? newestFeature : featureHistory[dist(randomGen)];
      std::array<double, 3> data { v.x, v.y, v.z };
      somPalette.addInstanceData(data);
    }
  }

  somPalette.update();

  emit(SOURCE_RANDOM, createVec4(randomDistrib(randomGen)));
  emit(SOURCE_RANDOM_LIGHT, createRandomLightVec4(randomDistrib(randomGen)));
  emit(SOURCE_RANDOM_DARK, createRandomDarkVec4(randomDistrib(randomGen)));
  emit(SOURCE_DARKEST, createVec4(0));
  emit(SOURCE_LIGHTEST, createVec4(SomPalette::size - 1));

  // convert RGB -> RG (float2), upload into FBO texture, emit FBO
  const ofFloatPixels& pixelsRef = somPalette.getPixelsRef();
  if (pixelsRef.getWidth() == 0 || pixelsRef.getHeight() == 0) return;
  ofFloatPixels converted = rgbToRG_Opponent(pixelsRef);
  ensureFieldTexture(converted.getWidth(), converted.getHeight());
  fieldTexture.loadData(converted);
  emit(SOURCE_FIELD, fieldTexture);
}

glm::vec4 SomPaletteMod::createVec4(int i) {
  ofFloatColor c = somPalette.getColor(i);
  return { c.r, c.g, c.b, 1.0f };
}

glm::vec4 SomPaletteMod::createRandomLightVec4(int i) {
  return createVec4((SomPalette::size - 1) - i / 2);
}

glm::vec4 SomPaletteMod::createRandomDarkVec4(int i) {
  return createVec4(i / 2);
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
      if (somPalette.nextPaletteIsReady() && v > 0.5f) {
        ofLogNotice("SomPaletteMod") << "SomPaletteMod switching palette";
        somPalette.switchPalette();
      }
      break;
    default:
      ofLogError("SomPaletteMod") << "Float receive for unknown sinkId " << sinkId;
  }
}

void SomPaletteMod::applyIntent(const Intent& intent, float strength) {
  // no-op for now
}

} // namespace ofxMarkSynth
