//
//  SandLineMod.cpp
//  example_sandlines
//
//  Created by Steve Meyfroidt on 10/05/2025.
//

#include "SandLineMod.hpp"


namespace ofxMarkSynth {


SandLineMod::SandLineMod(const std::string& name, const ModConfig&& config, const glm::vec2 fboSize)
: Mod { name, std::move(config) }
{
  fbo.allocate(fboSize.x, fboSize.y, GL_RGBA32F); // 32F to accommodate alpha, but this could be an optional thing
  fbo.getSource().clearColorBuffer(ofFloatColor(0.0, 0.0, 0.0, 0.0));
}

void SandLineMod::initParameters() {
  parameters.add(densityParameter);
  parameters.add(pointRadiusParameter);
  parameters.add(colorParameter);
//  parameters.add(fadeParameter);
}

void SandLineMod::drawSandLine(glm::vec2 p1, glm::vec2 p2) {
  auto distance = glm::distance(p1, p2);
  auto grains = static_cast<int>(distance * densityParameter * fbo.getWidth());
  for (int i = 0; i < grains; i++) {
    auto position = p1 + ofRandom() * (p2 - p1);
    auto radius = pointRadiusParameter / fbo.getWidth();
    auto offset = glm::vec2 { radius * 2.0, radius * 2.0 } -  glm::vec2 { radius, radius };
    position += offset;
    auto r = ofRandom(0.0, radius);
    ofDrawCircle(position, r);
  }
}

void SandLineMod::update() {
  fbo.getSource().begin();
  ofScale(fbo.getWidth(), fbo.getHeight());
  ofSetColor(colorParameter);
  ofFill();
  if (newPoints.size() > 1) {
    auto iter = newPoints.begin();
    while (iter < newPoints.end() - (newPoints.size() % 2)) {
      auto p1 = *iter; iter++;
      auto p2 = *iter; iter++;
      drawSandLine(p1, p2);
    }
    newPoints.erase(newPoints.begin(), iter);
  }
  fbo.getSource().end();
}

void SandLineMod::draw() {
  fbo.draw(0.0, 0.0);
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
