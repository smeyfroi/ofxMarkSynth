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
  parameters.add(alphaMultiplierParameter);
  parameters.add(softnessParameter);
}

void SoftCircleMod::update() {
  auto drawingLayerPtrOpt = getNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) return;
  auto fboPtr = drawingLayerPtrOpt.value()->fboPtr;

  float radius = radiusParameter;
  float radiusVariance = radiusVarianceParameter;
  float radiusVarianceScale = radiusVarianceScaleParameter;
  radius += radiusVariance * radiusVarianceScale;

  float multiplier = colorMultiplierParameter;
  ofFloatColor c = colorParameter;
  c *= multiplier;
  c.a *= alphaMultiplierParameter;
  
  float softness = softnessParameter;

  fboPtr->getSource().begin();
  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
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
    case SINK_AUDIO_TIMBRE_CHANGE:
      {
        float newRadiusVarianceScale = ofRandom(0.0, 100 * radiusParameter);
        ofLogNotice() << "SoftCircleMod::receive audio timbre change; new radius variance scale " << newRadiusVarianceScale;
        radiusVarianceScaleParameter = newRadiusVarianceScale;
      }
      break;
    default:
      ofLogError() << "float receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

float SoftCircleMod::bidToReceive(int sinkId) {
  switch (sinkId) {
    case SINK_AUDIO_TIMBRE_CHANGE:
      return 0.2;
    default:
      return 0.0;
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
