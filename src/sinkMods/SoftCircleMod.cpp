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
  parameters.add(colorParameter);
  parameters.add(colorMultiplierParameter);
  parameters.add(alphaMultiplierParameter);
  parameters.add(softnessParameter);
}

constexpr float AGENCY = 0.1f;

void SoftCircleMod::update() {
  auto drawingLayerPtrOpt = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) return;
  auto fboPtr = drawingLayerPtrOpt.value()->fboPtr;

  radiusParamController.update(AGENCY); // FIXME: hardcoded agency
  float radius = radiusParamController.value;

  float multiplier = colorMultiplierParameter;
  ofFloatColor c = colorParameter;
  c *= multiplier;
  c.a *= alphaMultiplierParameter;
  
  float softness = softnessParameter;

  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
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
      radiusParamController.update(value, AGENCY); // FIXME: hardcoded agency
      break;
    case SINK_POINT_COLOR_MULTIPLIER:
      colorMultiplierParameter = value;
      break;
    case SINK_POINT_SOFTNESS:
      softnessParameter = value;
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
    case SINK_POINT_COLOR:
      colorParameter = ofFloatColor { v.r, v.g, v.b, v.a };
      break;
    default:
      ofLogError() << "glm::vec4 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}


} // ofxMarkSynth
