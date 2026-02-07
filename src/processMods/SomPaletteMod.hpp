//  example_audio_palette
//
//  Created by Steve Meyfroidt on 06/05/2025.
//

#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <random>
#include <vector>

#include "ofxGui.h"

#include "core/Mod.hpp"
#include "ofxContinuousSomPalette.hpp"
#include "util/Oklab.h"

namespace ofxMarkSynth {

class SomPaletteMod : public Mod {

public:
  SomPaletteMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config);
  ~SomPaletteMod();
  void doneModLoad() override;
  float getAgency() const override;
  void update() override;
  void draw() override;
  bool keyPressed(int key) override;

  UiState captureUiState() const override;
  void restoreUiState(const UiState& state) override;

  RuntimeState captureRuntimeState() const override;
  void restoreRuntimeState(const RuntimeState& state) override;

  void receive(int sinkId, const glm::vec3& v) override;
  void receive(int sinkId, const float& v) override;
  void applyIntent(const Intent& intent, float strength) override;

  const ofTexture* getActivePaletteTexturePtr() const;
  const ofTexture* getNextPaletteTexturePtr() const;
  const ofTexture* getChipsTexturePtr() const;

  static constexpr int SINK_VEC3 = 1;
  static constexpr int SINK_SWITCH_PALETTE = 100;
  static constexpr int SOURCE_RANDOM = 2; // RGBA float color as vec4
  static constexpr int SOURCE_RANDOM_DARK = 3; // RGBA float color as vec4
  static constexpr int SOURCE_RANDOM_LIGHT = 4; // RGBA float color as vec4
  static constexpr int SOURCE_RANDOM_NOVELTY = 5; // RGBA float color as vec4
  static constexpr int SOURCE_DARKEST = 10; // RGBA float color as vec4
  static constexpr int SOURCE_LIGHTEST = 11; // RGBA float color as vec4
  static constexpr int SOURCE_FIELD = 1; // SOM as float field in RG pixels converted from RGB

  void onIterationsParameterChanged(float& value);
  void onColorizerParameterChanged(float& value);
  void onWindowSecsParameterChanged(float& value);

protected:
  void initParameters() override;

private:
  static constexpr int NOVELTY_CACHE_SIZE = 4;

  struct CachedNovelty {
    Oklab lab;
    ofFloatColor rgb;
    float chromaNoveltyScore { 0.0f }; // larger = more different from the main palette in hue/chroma
    int64_t lastSeenFrame { 0 };
  };

  struct PendingNovelty {
    Oklab lab;
    ofFloatColor rgb;
    float chromaNoveltyScore { 0.0f };
    int framesSeen { 0 };
    int64_t lastSeenFrame { 0 };
  };

  // Number of SOM training iterations per palette.
  // At 30fps and `TrainingStepsPerFrame` steps, time-to-converge ~= Iterations / (fps*steps).
  ofParameter<float> iterationsParameter { "Iterations", 4000.0f, 300.0f, 20000.0f };

  // Sliding timbre window length.
  ofParameter<float> windowSecsParameter { "WindowSecs", 10.0f, 2.0f, 60.0f };

  // Persistent chip memory duration as multiplier of WindowSecs.
  ofParameter<float> chipMemoryMultiplierParameter { "ChipMemoryMultiplier", 1.3f, 1.0f, 6.0f };

  // Fade-in from first received sample (avoid harsh startup flashes).
  ofParameter<float> startupFadeSecsParameter { "StartupFadeSecs", 2.0f, 0.0f, 10.0f };

  // Training multiplier: number of samples drawn from the sliding window per frame.
  ofParameter<int> trainingStepsPerFrameParameter { "TrainingStepsPerFrame", 8, 1, 40 };

  ofParameter<float> agencyFactorParameter { "AgencyFactor", 1.0f, 0.0f, 1.0f };

  // Chance that SOURCE_RANDOM emits a novelty cached color (if available).
  ofParameter<float> noveltyEmitChanceParameter { "NoveltyEmitChance", 0.35f, 0.0f, 1.0f };

  // Anti-collapse fallback: inject controlled variation into training samples when the input feature history has
  // very low variance (e.g. a sustained solo tone).
  ofParameter<float> antiCollapseJitterParameter { "AntiCollapseJitter", 0.06f, 0.0f, 0.2f };
  ofParameter<float> antiCollapseVarianceSecsParameter { "AntiCollapseVarianceSecs", 2.0f, 0.5f, 20.0f };
  ofParameter<float> antiCollapseVarianceThresholdParameter { "AntiCollapseVarianceThreshold", 0.0005f, 0.0f, 0.01f };
  ofParameter<float> antiCollapseDriftSpeedParameter { "AntiCollapseDriftSpeed", 0.12f, 0.0f, 1.0f };

  ofParameter<float> colorizerGrayGainParameter { "ColorizerGrayGain", 0.8f, 0.0f, 2.0f };
  ofParameter<float> colorizerChromaGainParameter { "ColorizerChromaGain", 2.5f, 0.0f, 4.0f };

  ContinuousSomPalette somPalette { 8, 8, 0.012f };

  // Feature history (last `WindowSecs` at ~30fps).
  int windowFrames { 450 };
  std::deque<glm::vec3> featureHistory;

  // Persistent chip set (Oklab) used for outputs.
  bool hasPersistentChips { false };
  std::array<Oklab, SomPalette::size> persistentChipsLab;
  std::array<ofFloatColor, SomPalette::size> persistentChipsRgb;
  std::array<int, SomPalette::size> persistentIndicesByLightness;

  // Novelty cache (rare colors that persist briefly).
  std::vector<CachedNovelty> noveltyCache;
  std::vector<PendingNovelty> pendingNovelty;

  int64_t paletteFrameCount { 0 };
  int64_t firstSampleFrameCount { -1 };
  float startupFadeFactor { 0.0f };

  std::vector<glm::vec3> newVecs;

  ofTexture fieldTexture; // RG float texture converted from RGB float pixels of the SOM
  void ensureFieldTexture(int w, int h);

  ofFloatPixels chipsPixels;
  ofTexture chipsTexture;
  void updateChipsTexture();

  std::ranlux24_base randomGen { 0 }; // fast generator with fixed seed
  std::uniform_int_distribution<> randomDistrib { 0, SomPalette::size - 1 };
  std::uniform_int_distribution<> randomHalfDistrib { 0, (SomPalette::size / 2) - 1 };
  std::uniform_real_distribution<float> random01Distrib { 0.0f, 1.0f };

  void updatePersistentChips(float dt);
  void updateNoveltyCache(const std::array<Oklab, SomPalette::size>& candidatesLab,
                          const std::array<ofFloatColor, SomPalette::size>& candidatesRgb,
                          const std::array<float, SomPalette::size>& noveltyScores,
                          float dt);

  int getPersistentDarkestIndex() const;
  int getPersistentLightestIndex() const;

  glm::vec3 computeRecentFeatureVarianceVec(int frames) const;
  float computeRecentFeatureVariance(int frames) const;
  float computeAntiCollapseFactor(const glm::vec3& varianceVec) const;
  glm::vec3 applyAntiCollapseJitter(const glm::vec3& v,
                                   float factor,
                                   const glm::vec3& varianceVec,
                                   float timeSecs,
                                   int step) const;
 
  glm::vec4 createVec4(int i);
  glm::vec4 createRandomVec4();
  glm::vec4 createRandomNoveltyVec4();
  glm::vec4 createRandomLightVec4();
  glm::vec4 createRandomDarkVec4();
};

} // ofxMarkSynth
