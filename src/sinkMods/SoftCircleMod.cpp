//
//  SoftCircleMod.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 25/05/2025.
//

#include "SoftCircleMod.hpp"


namespace ofxMarkSynth {


SoftCircleMod::SoftCircleMod(const std::string& name, const ModConfig&& config)
: Mod { name, std::move(config) }
{
  softCircleShader.load();
}

void SoftCircleMod::initParameters() {
  parameters.add(radiusParameter);
  parameters.add(radiusVarianceParameter);
  parameters.add(radiusVarianceScaleParameter);
  parameters.add(colorParameter);
  parameters.add(colorMultiplierParameter);
  parameters.add(softnessParameter);
}

void SoftCircleMod::update() {
  auto fboPtr = fboPtrs[0];
  if (fboPtr == nullptr) return;
  
  float radius = radiusParameter;
  float radiusVariance = radiusVarianceParameter;
  float radiusVarianceScale = radiusVarianceScaleParameter;
  radius += radiusVariance * radiusVarianceScale;

  float multiplier = colorMultiplierParameter;
  ofFloatColor c = colorParameter;
  c *= multiplier;
  
  float softness = softnessParameter;

  fboPtr->getSource().begin();
  std::for_each(newPoints.begin(),
                newPoints.end(),
                [this, fboPtr, radius, c, softness](const auto& p) {
    softCircleShader.render(p * fboPtr->getSize(), radius * fboPtr->getWidth(), c, softness);
  });
  fboPtr->getSource().end();
  
  newPoints.clear();
}

void SoftCircleMod::receive(int sinkId, const float& value) {
  switch (sinkId) {
    case SINK_POINT_RADIUS:
      radiusParameter = value;
      break;
    case SINK_POINT_RADIUS_VARIANCE:
      radiusVarianceParameter = value;
      break;
    case SINK_POINT_COLOR_MULTIPLIER:
      colorMultiplierParameter = value;
      break;
    case SINK_POINT_SOFTNESS:
      softnessParameter = value;
      break;
    default:
      ofLogError() << "float receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void SoftCircleMod::receive(int sinkId, const glm::vec2& point) {
  switch (sinkId) {
    case SINK_POINTS:
      newPoints.push_back(point);
      break;
    default:
      ofLogError() << "glm::vec2 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void SoftCircleMod::receive(int sinkId, const glm::vec4& v) {
  switch (sinkId) {
    case SINK_POINT_COLOR:
      colorParameter = ofFloatColor { v.r, v.g, v.b, v.a };
      break;
    default:
      ofLogError() << "glm::vec4 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}


} // ofxMarkSynth
