//
//  SmearMod.cpp
//  fingerprint-ipad-1
//
//  Created by Steve Meyfroidt on 01/08/2025.
//

#include "SmearMod.hpp"


namespace ofxMarkSynth {


SmearMod::SmearMod(const std::string& name, const ModConfig&& config)
: Mod { name, std::move(config) }
{
  smearShader.load();
}

void SmearMod::initParameters() {
  parameters.add(mixNewParameter);
  parameters.add(alphaMultiplierParameter);
  parameters.add(translateByParameter);
  parameters.add(field1MultiplierParameter);
  parameters.add(field1BiasParameter);
  parameters.add(field2MultiplierParameter);
  parameters.add(field2BiasParameter);
  
  parameters.add(gridSizeParameter);
  parameters.add(strategyParameter);
  parameters.add(jumpAmountParameter);
  parameters.add(borderWidthParameter);
  parameters.add(gridLevelsParameter);
  parameters.add(ghostBlendParameter);
  parameters.add(foldPeriodParameter);
}

void SmearMod::update() {
  auto drawingLayerPtrOpt = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) return;
  auto fboPtr = drawingLayerPtrOpt.value()->fboPtr;

  glm::vec2 translation { translateByParameter->x, translateByParameter->y };
  float mixNew = mixNewParameter;
  float alphaMultiplier = alphaMultiplierParameter;
  SmearShader::GridParameters gridParameters = {
    .gridSize = glm::vec2 { gridSizeParameter->x, gridSizeParameter->y },
    .strategy = strategyParameter,
    .jumpAmount = jumpAmountParameter,
    .borderWidth = borderWidthParameter,
    .gridLevels = gridLevelsParameter,
    .ghostBlend = ghostBlendParameter,
    .foldPeriod = glm::vec2 { foldPeriodParameter->x, foldPeriodParameter->y }
  };
  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  // TODO: make this more forgiving
  if (field2Fbo.isAllocated() && field1Fbo.isAllocated()) {
    smearShader.render(*fboPtr, translation, mixNew, alphaMultiplier,
                       field1Fbo.getTexture(), field1MultiplierParameter, field1BiasParameter,
                       field2Fbo.getTexture(), field2MultiplierParameter, field2BiasParameter,
                       gridParameters);
  } else if (field1Fbo.isAllocated()) {
    smearShader.render(*fboPtr, translation, mixNew, alphaMultiplier, field1Fbo.getTexture(), field1MultiplierParameter, field1BiasParameter);
  } else {
    smearShader.render(*fboPtr, translation, mixNew, alphaMultiplier);
  }
}

void SmearMod::receive(int sinkId, const float& value) {
  switch (sinkId) {
    case SINK_FLOAT:
      mixNewParameter = value;
      break;
    case SINK_CHANGE_LAYER:
      if (value > 0.9) {
        ofLogNotice() << "SmearMod::SINK_CHANGE_LAYER: disable layer";
        disableDrawingLayer();
      } else if (value > 0.6) { // FIXME: temp until connections have weights
        ofLogNotice() << "SmearMod::SINK_CHANGE_LAYER: changing layer";
        changeDrawingLayer();
      } else if (value > 0.3) {
        // higher chance to return to default layer
        ofLogNotice() << "SmearMod::SINK_CHANGE_LAYER: default layer";
        resetDrawingLayer();
      }
      break;
    default:
      ofLogError() << "float receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void SmearMod::receive(int sinkId, const glm::vec2& v) {
  switch (sinkId) {
    case SINK_VEC2:
      translateByParameter = v;
      break;
    default:
      ofLogError() << "glm::vec2 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void SmearMod::receive(int sinkId, const ofFbo& value) {
  switch (sinkId) {
    case SINK_FIELD_1_FBO:
      field1Fbo = value;
      break;
    case SINK_FIELD_2_FBO:
      field2Fbo = value;
      break;
    default:
      ofLogError() << "ofFbo receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}


} // ofxMarkSynth
