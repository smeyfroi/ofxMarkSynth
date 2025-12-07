//
//  SoftCircleMod.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 25/05/2025.
//

#include "SoftCircleMod.hpp"
#include "IntentMapping.hpp"
#include "../IntentMapper.hpp"



namespace ofxMarkSynth {



SoftCircleMod::SoftCircleMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config)
: Mod { synthPtr, name, std::move(config) }
{
  softCircleShader.load();

  sinkNameIdMap = {
    { "Point", SINK_POINTS },
    { radiusParameter.getName(), SINK_RADIUS },
    { colorParameter.getName(), SINK_COLOR },
    { colorMultiplierParameter.getName(), SINK_COLOR_MULTIPLIER },
    { alphaMultiplierParameter.getName(), SINK_ALPHA_MULTIPLIER },
    { softnessParameter.getName(), SINK_SOFTNESS },
    { "ChangeLayer", SINK_CHANGE_LAYER }
  };
  
  registerControllerForSource(radiusParameter, radiusController);
  registerControllerForSource(colorParameter, colorController);
  registerControllerForSource(colorMultiplierParameter, colorMultiplierController);
  registerControllerForSource(alphaMultiplierParameter, alphaMultiplierController);
  registerControllerForSource(softnessParameter, softnessController);
}

void SoftCircleMod::initParameters() {
  parameters.add(radiusParameter);
  parameters.add(colorParameter);
  parameters.add(colorMultiplierParameter);
  parameters.add(alphaMultiplierParameter);
  parameters.add(softnessParameter);
  parameters.add(falloffParameter);
  parameters.add(agencyFactorParameter);
}

float SoftCircleMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

void SoftCircleMod::update() {
  syncControllerAgencies();
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

  int falloff = falloffParameter.get();
  
  fboPtr->getSource().begin();
  
  ofPushStyle();
  if (falloff == 1) {
    // Dab: premultiplied alpha blend for proper compositing without halos
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  } else {
    // Glow: standard alpha blending
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  }
  
  std::for_each(newPoints.begin(),
                newPoints.end(),
                [&](const auto& p) {
    softCircleShader.render(p * fboPtr->getSize(), radius * fboPtr->getWidth(), color, softness, falloff);
  });
  
  ofPopStyle();
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
        ofLogNotice("SoftCircleMod") << "SoftCircleMod::SINK_CHANGE_LAYER: changing layer";
        changeDrawingLayer();
      }
      break;
    default:
      ofLogError("SoftCircleMod") << "Float receive for unknown sinkId " << sinkId;
  }
}

void SoftCircleMod::receive(int sinkId, const glm::vec2& point) {
  switch (sinkId) {
    case SINK_POINTS:
      newPoints.push_back(point);
      break;
    default:
      ofLogError("SoftCircleMod") << "glm::vec2 receive for unknown sinkId " << sinkId;
  }
}

void SoftCircleMod::receive(int sinkId, const glm::vec4& v) {
  switch (sinkId) {
    case SINK_COLOR:
      colorController.updateAuto(ofFloatColor { v.r, v.g, v.b, v.a }, getAgency());
      break;
    default:
      ofLogError("SoftCircleMod") << "glm::vec4 receive for unknown sinkId " << sinkId;
  }
}

void SoftCircleMod::applyIntent(const Intent& intent, float strength) {
  IntentMap im(intent);
  im.E().exp(radiusController, strength);
  im.D().lin(colorMultiplierController, strength);
  im.D().lin(alphaMultiplierController, strength);
}



} // ofxMarkSynth
