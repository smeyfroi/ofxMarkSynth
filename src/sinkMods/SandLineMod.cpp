//
//  SandLineMod.cpp
//  example_sandlines
//
//  Created by Steve Meyfroidt on 10/05/2025.
//

#include "SandLineMod.hpp"


namespace ofxMarkSynth {


SandLineMod::SandLineMod(Synth* synthPtr, const std::string& name, const ModConfig&& config)
: Mod { synthPtr, name, std::move(config) }
{}

void SandLineMod::initParameters() {
  parameters.add(densityParameter);
  parameters.add(pointRadiusParameter);
  parameters.add(colorParameter);
  parameters.add(alphaMultiplierParameter);
  parameters.add(stdDevAlongParameter);
  parameters.add(stdDevPerpendicularParameter);
}

void SandLineMod::drawSandLine(glm::vec2 p1, glm::vec2 p2, float drawScale) {
  static std::mt19937 generator(std::random_device{}());

  glm::vec2 lineVector = p2 - p1;
  float lineLength = glm::length(lineVector);
  glm::vec2 midpoint = (p1 + p2) * 0.5f;
  glm::vec2 unitDirection = glm::normalize(lineVector);
  glm::vec2 perpendicular { -unitDirection.y, unitDirection.x };
  
  std::normal_distribution<float> alongDist(0.0f, stdDevAlongParameter * lineLength);
  std::normal_distribution<float> perpDist(0.0f, stdDevPerpendicularParameter * lineLength);

  auto grains = static_cast<int>(lineLength * densityParameter * drawScale);
  float maxRadius = pointRadiusParameter / drawScale;

  for (int i = 0; i < grains; i++) {
    float offsetAlong = alongDist(generator);

    // Clamp to line bounds (optional - remove for unbounded distribution)
//    float halfLength = lineLength * 0.5f;
//    offset = glm::clamp(offset, -halfLength, halfLength);

    float offsetPerp = perpDist(generator);

    glm::vec2 point = midpoint + (offsetAlong * unitDirection) + (offsetPerp * perpendicular);
    auto r = ofRandom(0.0, maxRadius);
    ofDrawCircle(point, r);
  }
}

void SandLineMod::update() {
  auto drawingLayerPtrOpt = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) return;
  auto fboPtr = drawingLayerPtrOpt.value()->fboPtr;

  float drawScale = fboPtr->getWidth();
  fboPtr->getSource().begin();
  ofScale(fboPtr->getWidth(), fboPtr->getHeight());
  ofEnableBlendMode(OF_BLENDMODE_ALPHA);

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
      Mod::receive(sinkId, value);
//      ofLogError() << "float receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
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
