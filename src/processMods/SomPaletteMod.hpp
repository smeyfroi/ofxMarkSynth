//
//  SomPaletteMod.hpp
//  example_audio_palette
//
//  Created by Steve Meyfroidt on 06/05/2025.
//

#pragma once

#include "ofxGui.h"
#include "Mod.hpp"
#include "ofxContinuousSomPalette.hpp"
#include <random>


namespace ofxMarkSynth {


class SomPaletteMod : public Mod {

public:
  SomPaletteMod(Synth* synthPtr, const std::string& name, const ModConfig&& config);
  void update() override;
  void draw() override;
  bool keyPressed(int key) override;
  void receive(int sinkId, const glm::vec3& v) override;
  void receive(int sinkId, const float& v) override;

  static constexpr int SINK_VEC3 = 1;
  static constexpr int SINK_SWITCH_PALETTE = 100;
  static constexpr int SOURCE_RANDOM = 2; // RGBA float color as vec4
  static constexpr int SOURCE_RANDOM_DARK = 3; // RGBA float color as vec4
  static constexpr int SOURCE_RANDOM_LIGHT = 4; // RGBA float color as vec4
  static constexpr int SOURCE_DARKEST = 10; // RGBA float color as vec4
  static constexpr int SOURCE_FIELD = 1; // SOM as float field in RG pixels converted from RGB

protected:
  void initParameters() override;

private:
//  ofParameter<float> learningRateParameter { "LearningRate", 0.01, 0.0, 1.0 };
  ofParameter<float> iterationsParameter { "Iterations", 3000.0, 1000.0, 100000.0 };

  ContinuousSomPalette somPalette { 16, 16, 0.02 };

  std::vector<glm::vec3> newVecs;
  
  ofFbo fieldFbo; // RG float texture converted from RGB float pixels of the SOM
  void ensureFieldFbo(int w, int h);

  std::ranlux24_base randomGen { 0 }; // fast generator with fixed seed
  std::uniform_int_distribution<> randomDistrib { 0, SomPalette::size - 1 };
  glm::vec4 createVec4(int i);
  glm::vec4 createRandomLightVec4(int i);
  glm::vec4 createRandomDarkVec4(int i);
};


} // ofxMarkSynth
