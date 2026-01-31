//
//  VectorMagnitudeMod.cpp
//  ofxMarkSynth
//
//  Created by Steve Meyfroidt on 10/01/2026.
//

#include "VectorMagnitudeMod.hpp"

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>



namespace ofxMarkSynth {



namespace {

VectorMagnitudeMod::Components parseComponents(const std::string& s) {
  if (s == "xy" || s == "XY") return VectorMagnitudeMod::Components::XY;
  if (s == "zw" || s == "ZW") return VectorMagnitudeMod::Components::ZW;
  if (s == "xyz" || s == "XYZ") return VectorMagnitudeMod::Components::XYZ;
  if (s == "xyzw" || s == "XYZW") return VectorMagnitudeMod::Components::XYZW;

  return VectorMagnitudeMod::Components::ZW;
}

} // namespace



VectorMagnitudeMod::VectorMagnitudeMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config)
: Mod { synthPtr, name, std::move(config) }
{
  sinkNameIdMap = {
    { "Vec2", SINK_VEC2 },
    { "Vec3", SINK_VEC3 },
    { "Vec4", SINK_VEC4 },
    { "PointVelocity", SINK_VEC4 },
  };

  sourceNameIdMap = {
    { "MeanScalar", SOURCE_MEAN_SCALAR },
    { "MaxScalar", SOURCE_MAX_SCALAR },
  };
}

void VectorMagnitudeMod::initParameters() {
  parameters.add(minParameter);
  parameters.add(maxParameter);
  parameters.add(meanSmoothingParameter);
  parameters.add(maxSmoothingParameter);
  parameters.add(decayWhenNoInputParameter);
  parameters.add(componentsParameter);
}

void VectorMagnitudeMod::update() {
  syncControllerAgencies();

  float mean = 0.0f;
  float max = 0.0f;

  lastSampleCount = sampleCount;

  if (sampleCount > 0) {
    mean = sumMagnitude / static_cast<float>(sampleCount);
    max = maxMagnitude;
  } else {
    // If we have no input samples this frame, decay toward 0.
    meanState *= decayWhenNoInputParameter;
    maxState *= decayWhenNoInputParameter;
  }

  lastRawMean = mean;
  lastRawMax = max;

  sumMagnitude = 0.0f;
  maxMagnitude = 0.0f;
  sampleCount = 0;

  float meanN = normalise(mean);
  float maxN = normalise(max);

  meanN = smooth(meanN, meanState, meanSmoothingParameter);
  maxN = smooth(maxN, maxState, maxSmoothingParameter);

  lastMeanOut = meanN;
  lastMaxOut = maxN;

  emit(SOURCE_MEAN_SCALAR, meanN);
  emit(SOURCE_MAX_SCALAR, maxN);
}

VectorMagnitudeMod::Components VectorMagnitudeMod::getComponentsForVecSize(int vecSize) const {
  auto requested = parseComponents(componentsParameter);

  switch (vecSize) {
    case 2:
      return Components::XY;
    case 3:
      if (requested == Components::XYZ) return Components::XYZ;
      return Components::XY;
    case 4:
      if (requested == Components::XY || requested == Components::ZW || requested == Components::XYZW) return requested;
      return Components::ZW;
    default:
      return Components::ZW;
  }
}

float VectorMagnitudeMod::getMagnitude(glm::vec2 v, Components components) {
  (void)components;
  return glm::length(v);
}

float VectorMagnitudeMod::getMagnitude(glm::vec3 v, Components components) {
  switch (components) {
    case Components::XYZ:
      return glm::length(v);
    case Components::XY:
    default:
      return glm::length(glm::vec2 { v.x, v.y });
  }
}

float VectorMagnitudeMod::getMagnitude(glm::vec4 v, Components components) {
  switch (components) {
    case Components::XY:
      return glm::length(glm::vec2 { v.x, v.y });
    case Components::XYZW:
      return glm::length(v);
    case Components::ZW:
    default:
      return glm::length(glm::vec2 { v.z, v.w });
  }
}

float VectorMagnitudeMod::normalise(float value) const {
  float minValue = minParameter;
  float maxValue = maxParameter;
  if (maxValue <= minValue) {
    maxValue = minValue + 0.000001f;
  }

  float n = (value - minValue) / (maxValue - minValue);
  return std::clamp(n, 0.0f, 1.0f);
}

float VectorMagnitudeMod::smooth(float value, float& state, float smoothing) const {
  // smoothing = 0 -> immediate, 1 -> fully held.
  smoothing = std::clamp(smoothing, 0.0f, 1.0f);
  state = state * smoothing + value * (1.0f - smoothing);
  return state;
}

void VectorMagnitudeMod::addSample(float magnitude) {
  sumMagnitude += magnitude;
  maxMagnitude = std::max(maxMagnitude, magnitude);
  sampleCount += 1;
}

void VectorMagnitudeMod::receive(int sinkId, const glm::vec2& v) {
  if (sinkId != SINK_VEC2) return;
  addSample(getMagnitude(v, getComponentsForVecSize(2)));
}

void VectorMagnitudeMod::receive(int sinkId, const glm::vec3& v) {
  if (sinkId != SINK_VEC3) return;
  addSample(getMagnitude(v, getComponentsForVecSize(3)));
}

void VectorMagnitudeMod::receive(int sinkId, const glm::vec4& v) {
  if (sinkId != SINK_VEC4) return;
  addSample(getMagnitude(v, getComponentsForVecSize(4)));
}



} // namespace ofxMarkSynth
