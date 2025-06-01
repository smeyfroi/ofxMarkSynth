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
  SomPaletteMod(const std::string& name, const ModConfig&& config);
  void update() override;
  void draw() override;
  bool keyPressed(int key) override;
  void receive(int sinkId, const glm::vec3& v) override;

  static constexpr int SINK_VEC3 = 1;
  static constexpr int SOURCE_RANDOM_VEC4 = 2; // RGBA float color
  static constexpr int SOURCE_RANDOM_DARK_VEC4 = 3; // RGBA float color
  static constexpr int SOURCE_RANDOM_LIGHT_VEC4 = 4; // RGBA float color

protected:
  void initParameters() override;

private:
//  ofParameter<float> learningRateParameter { "LearningRate", 0.01, 0.0, 1.0 };
//  ofParameter<float> iterationsParameter { "Iterations", 10000.0, 1000.0, 100000.0 };

  ContinuousSomPalette somPalette { 16, 16, 0.02, 4000 };

  std::vector<glm::vec3> newVecs;
  
  std::ranlux24_base randomGen { 0 }; // fast generator with fixed seed
  std::uniform_int_distribution<> randomDistrib { 0, SomPalette::size - 1 };
  glm::vec4 createRandomVec4(int i);
  glm::vec4 createRandomLightVec4(int i);
  glm::vec4 createRandomDarkVec4(int i);
};


} // ofxMarkSynth
