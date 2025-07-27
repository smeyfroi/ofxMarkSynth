//
//  DrawPointsMod.cpp
//  example_points
//
//  Created by Steve Meyfroidt on 05/05/2025.
//

#include "DrawPointsMod.hpp"


namespace ofxMarkSynth {


DrawPointsMod::DrawPointsMod(const std::string& name, const ModConfig&& config)
: Mod { name, std::move(config) }
{}

void DrawPointsMod::initParameters() {
  parameters.add(pointRadiusParameter);
  parameters.add(pointRadiusVarianceParameter);
  parameters.add(pointRadiusVarianceScaleParameter);
  parameters.add(colorParameter);
  parameters.add(colorMultiplierParameter);
}

void DrawPointsMod::update() {
  auto fboPtr = fboPtrs[0];
  if (fboPtr == nullptr) return;
  fboPtr->getSource().begin();
  ofScale(fboPtr->getWidth(), fboPtr->getHeight());
  ofFill();
  
  float multiplier = colorMultiplierParameter;
  ofFloatColor c = colorParameter;
  c *= multiplier;
  ofSetColor(c);
  ofSetLineWidth(0);
  
  float pointRadius = pointRadiusParameter;
  float pointRadiusVariance = pointRadiusVarianceParameter;
  float pointRadiusVarianceScale = pointRadiusVarianceScaleParameter;
  pointRadius += pointRadiusVariance * pointRadiusVarianceScale;
  
  std::for_each(newPoints.begin(), newPoints.end(), [pointRadius](const auto& p) {
    ofDrawCircle(p, pointRadius);
  });
  newPoints.clear();

  fboPtr->getSource().end();
}

void DrawPointsMod::receive(int sinkId, const float& value) {
  switch (sinkId) {
    case SINK_POINT_RADIUS:
      pointRadiusParameter = value;
      break;
    case SINK_POINT_RADIUS_VARIANCE:
      pointRadiusVarianceParameter = value;
      break;
    default:
      ofLogError() << "float receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void DrawPointsMod::receive(int sinkId, const glm::vec2& point) {
  switch (sinkId) {
    case SINK_POINTS:
      newPoints.push_back(point);
      break;
    default:
      ofLogError() << "glm::vec2 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void DrawPointsMod::receive(int sinkId, const glm::vec4& v) {
  switch (sinkId) {
    case SINK_POINT_COLOR:
      colorParameter = ofFloatColor { v.r, v.g, v.b, v.a };
      break;
    default:
      ofLogError() << "glm::vec4 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}


} // ofxMarkSynth
