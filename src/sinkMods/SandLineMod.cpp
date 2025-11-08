//
//  SandLineMod.cpp
//  example_sandlines
//
//  Created by Steve Meyfroidt on 10/05/2025.
//

#include "SandLineMod.hpp"
#include "IntentMapping.hpp"



namespace ofxMarkSynth {



SandLineMod::SandLineMod(Synth* synthPtr, const std::string& name, const ModConfig&& config)
: Mod { synthPtr, name, std::move(config) }
{
  sinkNameIdMap = {
    { "points", SINK_POINTS },
    { pointRadiusParameter.getName(), SINK_POINT_RADIUS },
    { colorParameter.getName(), SINK_POINT_COLOR }
  };
}

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
  
  std::normal_distribution<float> alongDist(0.0f, stdDevAlongController.value * lineLength);
  std::normal_distribution<float> perpDist(0.0f, stdDevPerpendicularController.value * lineLength);

  auto grains = static_cast<int>(lineLength * densityController.value * drawScale);
  float maxRadius = pointRadiusController.value / drawScale;

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
  densityController.update();
  pointRadiusController.update();
  colorController.update();
  alphaMultiplierController.update();
  stdDevAlongController.update();
  stdDevPerpendicularController.update();
  
  auto drawingLayerPtrOpt = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) return;
  auto fboPtr = drawingLayerPtrOpt.value()->fboPtr;

  float drawScale = fboPtr->getWidth();
  fboPtr->getSource().begin();
  ofScale(fboPtr->getWidth(), fboPtr->getHeight());
  ofEnableBlendMode(OF_BLENDMODE_ALPHA);

  ofFloatColor c = colorController.value;
  c.a *= alphaMultiplierController.value;
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
      pointRadiusController.updateAuto(value, getAgency());
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
      colorController.updateAuto(ofFloatColor { v.r, v.g, v.b, v.a }, getAgency());
      break;
    default:
      ofLogError() << "glm::vec4 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void SandLineMod::applyIntent(const Intent& intent, float strength) {
  if (strength < 0.01) return;
  
  // Density + Granularity → Density
  float densityI = exponentialMap(intent.getEnergy() * intent.getGranularity(), densityController);
  densityController.updateIntent(densityI, strength);

  // Granularity → Point Radius
  float radiusI = exponentialMap(intent.getGranularity(), 0.5f, 16.0f, 3.0f);
  pointRadiusController.updateIntent(radiusI, strength);
  
  ofFloatColor color = ofxMarkSynth::energyToColor(intent);
  color.setBrightness(ofxMarkSynth::structureToBrightness(intent));
  color.setSaturation(intent.getEnergy() * (1.0f - intent.getStructure()));
  color.a = 1.0f;
  colorController.updateIntent(color, strength);
  
  // Structure → Alpha Multiplier
  alphaMultiplierController.updateIntent(ofxMarkSynth::linearMap(intent.getStructure(), alphaMultiplierController), strength);
  
  // Inverse Structure → Std Dev Along
  stdDevAlongController.updateIntent(ofxMarkSynth::inverseMap(intent.getStructure(), stdDevAlongController), strength);
  
  // Chaos → Std Dev Perpendicular
  stdDevPerpendicularController.updateIntent(ofxMarkSynth::linearMap(intent.getChaos(), stdDevPerpendicularController), strength);
}



} // ofxMarkSynth
