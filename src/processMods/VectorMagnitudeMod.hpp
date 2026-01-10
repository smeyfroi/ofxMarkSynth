//
//  VectorMagnitudeMod.hpp
//  ofxMarkSynth
//
//  Created by Steve Meyfroidt on 10/01/2026.
//

#pragma once

#include "core/Mod.hpp"



namespace ofxMarkSynth {



class VectorMagnitudeMod : public Mod {

public:
  VectorMagnitudeMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config);

  void update() override;

  void receive(int sinkId, const glm::vec2& v) override;
  void receive(int sinkId, const glm::vec3& v) override;
  void receive(int sinkId, const glm::vec4& v) override;

  static constexpr int SINK_VEC2 = 10;
  static constexpr int SINK_VEC3 = 11;
  static constexpr int SINK_VEC4 = 12;

  static constexpr int SOURCE_MEAN_SCALAR = 20;
  static constexpr int SOURCE_MAX_SCALAR = 21;

protected:
  void initParameters() override;

public:
  enum class Components { XY, ZW, XYZ, XYZW };

private:
  Components getComponentsForVecSize(int vecSize) const;

  static float getMagnitude(glm::vec2 v, Components components);
  static float getMagnitude(glm::vec3 v, Components components);
  static float getMagnitude(glm::vec4 v, Components components);

  float normalise(float value) const;
  float smooth(float value, float& state, float smoothing) const;

  void addSample(float magnitude);

  float sumMagnitude { 0.0f };
  float maxMagnitude { 0.0f };
  int sampleCount { 0 };

  float meanState { 0.0f };
  float maxState { 0.0f };

  ofParameter<float> minParameter { "Min", 0.0f, 0.0f, 1.0f };
  ofParameter<float> maxParameter { "Max", 0.02f, 0.00001f, 1.0f };
  ofParameter<float> meanSmoothingParameter { "MeanSmoothing", 0.9f, 0.0f, 1.0f };
  ofParameter<float> maxSmoothingParameter { "MaxSmoothing", 0.85f, 0.0f, 1.0f };
  ofParameter<float> decayWhenNoInputParameter { "DecayWhenNoInput", 0.95f, 0.0f, 1.0f };
  ofParameter<std::string> componentsParameter { "Components", "zw" };
};



} // namespace ofxMarkSynth
