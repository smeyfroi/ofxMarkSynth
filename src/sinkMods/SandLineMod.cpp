//
//  SandLineMod.cpp
//  example_sandlines
//
//  Created by Steve Meyfroidt on 10/05/2025.
//

#include "SandLineMod.hpp"
#include "core/IntentMapping.hpp"
#include "core/IntentMapper.hpp"



namespace ofxMarkSynth {



SandLineMod::SandLineMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config)
: Mod { synthPtr, name, std::move(config) }
{
  sinkNameIdMap = {
    { "Point", SINK_POINTS },
    { pointRadiusParameter.getName(), SINK_POINT_RADIUS },
    { colorParameter.getName(), SINK_POINT_COLOR }
  };
  
  registerControllerForSource(densityParameter, densityController);
  registerControllerForSource(pointRadiusParameter, pointRadiusController);
  registerControllerForSource(colorParameter, colorController);
  registerControllerForSource(alphaMultiplierParameter, alphaMultiplierController);
  registerControllerForSource(stdDevAlongParameter, stdDevAlongController);
  registerControllerForSource(stdDevPerpendicularParameter, stdDevPerpendicularController);
}


void SandLineMod::initParameters() {
  parameters.add(densityParameter);
  parameters.add(pointRadiusParameter);
  parameters.add(colorParameter);
  parameters.add(alphaMultiplierParameter);
  parameters.add(stdDevAlongParameter);
  parameters.add(stdDevPerpendicularParameter);
  parameters.add(agencyFactorParameter);
}

float SandLineMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
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
  syncControllerAgencies();
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
  ofPushStyle();
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
  ofPopStyle();
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
      ofLogError("SandLineMod") << "glm::vec2 receive for unknown sinkId " << sinkId;
  }
}

void SandLineMod::receive(int sinkId, const glm::vec4& v) {
  switch (sinkId) {
    case SINK_POINT_COLOR:
      colorController.updateAuto(ofFloatColor { v.r, v.g, v.b, v.a }, getAgency());
      break;
    default:
      ofLogError("SandLineMod") << "glm::vec4 receive for unknown sinkId " << sinkId;
  }
}

void SandLineMod::applyIntent(const Intent& intent, float strength) {
  IntentMap im(intent);
  
  (im.E() * im.G()).exp(densityController, strength);
  im.G().exp(pointRadiusController, strength, 1.0f, 16.0f, 3.0f);
  
  // Color composition
  ofFloatColor color = ofxMarkSynth::energyToColor(intent);
  color.setBrightness(ofxMarkSynth::structureToBrightness(intent));
  color.setSaturation((im.E() * im.S().inv()).get());
  color.a = 1.0f;
  colorController.updateIntent(color, strength, "E->color, S->bright, E*(1-S)->sat");
  
  im.S().inv().lin(stdDevAlongController, strength);
  im.C().lin(stdDevPerpendicularController, strength);
}



} // ofxMarkSynth
