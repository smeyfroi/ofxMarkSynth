//
//  SandLineMod.cpp
//  example_sandlines
//
//  Created by Steve Meyfroidt on 10/05/2025.
//

#include "SandLineMod.hpp"


namespace ofxMarkSynth {


SandLineMod::SandLineMod(const std::string& name, const ModConfig&& config)
: Mod { name, std::move(config) }
{}

void SandLineMod::initParameters() {
  parameters.add(densityParameter);
  parameters.add(pointRadiusParameter);
  parameters.add(colorParameter);
  parameters.add(alphaMultiplierParameter);
}

void SandLineMod::drawSandLine(glm::vec2 p1, glm::vec2 p2, float drawScale) {
  auto distance = glm::distance(p1, p2);
  auto grains = static_cast<int>(distance * densityParameter * drawScale);
  for (int i = 0; i < grains; i++) {
    auto position = p1 + ofRandom() * (p2 - p1);
    auto radius = pointRadiusParameter / drawScale;
    auto offset = glm::vec2 { radius * 2.0, radius * 2.0 } -  glm::vec2 { radius, radius };
    position += offset;
    auto r = ofRandom(0.0, radius);
    ofDrawCircle(position, r);
  }
}

void SandLineMod::update() {
  auto fboPtr = fboPtrs[0];
  if (fboPtr == nullptr) return;

  float drawScale = fboPtr->getWidth();
  fboPtr->getSource().begin();
  ofScale(fboPtr->getWidth(), fboPtr->getHeight());
  ofEnableBlendMode(OF_BLENDMODE_ADD);

  ofFloatColor c = colorParameter;
  c.a *= alphaMultiplierParameter;
  ofSetColor(c);

  ofFill();
  if (newPoints.size() > 1) {
    auto iter = newPoints.begin();
    while (iter < newPoints.end() - (newPoints.size() % 2)) {
      auto p1 = *iter; iter++;
      auto p2 = *iter; iter++;
      drawSandLine(p1, p2, drawScale);
    }
    newPoints.erase(newPoints.begin(), iter);
  }
  fboPtr->getSource().end();
}

void SandLineMod::receive(int sinkId, const float& value) {
  switch (sinkId) {
    case SINK_POINT_RADIUS:
      pointRadiusParameter = value;
      break;
    default:
      ofLogError() << "float receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void SandLineMod::receive(int sinkId, const glm::vec2& point) {
  switch (sinkId) {
    case SINK_POINTS:
      newPoints.push_back(point);
      break;
    default:
      ofLogError() << "glm::vec2 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void SandLineMod::receive(int sinkId, const glm::vec4& v) {
  switch (sinkId) {
    case SINK_POINT_COLOR:
      colorParameter = ofFloatColor { v.r, v.g, v.b, v.a };
      break;
    default:
      ofLogError() << "glm::vec4 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}


} // ofxMarkSynth
