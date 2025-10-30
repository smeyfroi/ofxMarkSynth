//
//  SoftCircleMod.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 25/05/2025.
//

#include "SoftCircleMod.hpp"


namespace ofxMarkSynth {


SoftCircleMod::SoftCircleMod(Synth* synthPtr, const std::string& name, const ModConfig&& config)
: Mod { synthPtr, name, std::move(config) }
{
  softCircleShader.load();

  sinkNameIdMap = {
    { "points", SINK_POINTS },
    { "radius", SINK_RADIUS },
    { "color", SINK_COLOR },
    { "colorMultiplier", SINK_COLOR_MULTIPLIER },
    { "alphaMultiplier", SINK_ALPHA_MULTIPLIER },
    { "softness", SINK_SOFTNESS }
  };
}

void SoftCircleMod::initParameters() {
  parameters.add(radiusParameter);
  parameters.add(colorParameter);
  parameters.add(colorMultiplierParameter);
  parameters.add(alphaMultiplierParameter);
  parameters.add(softnessParameter);
  parameters.add(agencyFactorParameter);
}

float SoftCircleMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

void SoftCircleMod::update() {
  auto drawingLayerPtrOpt = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) return;
  auto fboPtr = drawingLayerPtrOpt.value()->fboPtr;

  radiusController.update();
  float radius = radiusController.value;

  colorController.update();
  ofFloatColor color = colorController.value;

  colorMultiplierController.update();
  float multiplier = colorMultiplierController.value;
  color *= multiplier;
  
  alphaMultiplierController.update();
  float alphaMultiplier = alphaMultiplierController.value;
  color.a *= alphaMultiplierParameter;
  
  softnessController.update();
  float softness = softnessController.value;

  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  fboPtr->getSource().begin();
  std::for_each(newPoints.begin(),
                newPoints.end(),
                [&](const auto& p) {
    softCircleShader.render(p * fboPtr->getSize(), radius * fboPtr->getWidth(), color, softness);
  });
  fboPtr->getSource().end();
  
  newPoints.clear();
}

void SoftCircleMod::receive(int sinkId, const float& value) {
  switch (sinkId) {
    case SINK_RADIUS:
      radiusController.updateAuto(value, getAgency());
      break;
    case SINK_COLOR_MULTIPLIER:
      colorMultiplierController.updateAuto(value, getAgency());
      break;
    case SINK_ALPHA_MULTIPLIER:
      alphaMultiplierController.updateAuto(value, getAgency());
      break;
    case SINK_SOFTNESS:
      softnessController.updateAuto(value, getAgency());
      break;
    case SINK_CHANGE_LAYER:
      if (value > 0.5) { // FIXME: temp until connections have weights
        ofLogNotice() << "SoftCircleMod::SINK_CHANGE_LAYER: changing layer";
        changeDrawingLayer();
      }
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
    case SINK_COLOR:
      colorController.updateAuto(ofFloatColor { v.r, v.g, v.b, v.a }, getAgency());
      break;
    default:
      ofLogError() << "glm::vec4 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void SoftCircleMod::applyIntent(const Intent& intent, float strength) {
  // Energy → Radius
  float radiusI = linearMap(intent.getEnergy(), 0.002f, 0.032f);
  radiusController.updateIntent(radiusI, strength);

  // Energy → Color; Density → Alpha
  ofFloatColor colorI = energyToColor(intent);
  colorI.a = linearMap(intent.getDensity(), 0.02f, 0.3f);
  colorController.updateIntent(colorI, strength);

  // Density → Alpha Multiplier
  float alphaI = linearMap(intent.getDensity(), 0.02f, 0.3f);
  alphaMultiplierController.updateIntent(alphaI, strength);
}


} // ofxMarkSynth
