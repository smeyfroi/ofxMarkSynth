//
//  SmearMod.cpp
//  fingerprint-ipad-1
//
//  Created by Steve Meyfroidt on 01/08/2025.
//

#include "SmearMod.hpp"
#include "IntentMapping.hpp"



namespace ofxMarkSynth {



SmearMod::SmearMod(Synth* synthPtr, const std::string& name, const ModConfig&& config)
: Mod { synthPtr, name, std::move(config) }
{
  smearShader.load();
  
  sinkNameIdMap = {
    { "vec2", SINK_VEC2 },
    { "float", SINK_FLOAT },
    { "field1Fbo", SINK_FIELD_1_FBO },
    { "field2Fbo", SINK_FIELD_2_FBO }
  };
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
  mixNewController.update();
  alphaMultiplierController.update();
  field1MultiplierController.update();
  field2MultiplierController.update();
  jumpAmountController.update();
  borderWidthController.update();
  ghostBlendController.update();
  auto drawingLayerPtrOpt = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) return;
  auto fboPtr = drawingLayerPtrOpt.value()->fboPtr;

  glm::vec2 translation { translateByParameter->x, translateByParameter->y };
  float mixNew = mixNewController.value;
  float alphaMultiplier = alphaMultiplierController.value;
  SmearShader::GridParameters gridParameters = {
    .gridSize = glm::vec2 { gridSizeParameter->x, gridSizeParameter->y },
    .strategy = strategyParameter,
    .jumpAmount = jumpAmountController.value,
    .borderWidth = borderWidthController.value,
    .gridLevels = gridLevelsParameter,
    .ghostBlend = ghostBlendController.value,
    .foldPeriod = glm::vec2 { foldPeriodParameter->x, foldPeriodParameter->y }
  };
  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  // TODO: make this more forgiving
  if (field2Fbo.isAllocated() && field1Fbo.isAllocated()) {
    smearShader.render(*fboPtr, translation, mixNew, alphaMultiplier,
                       field1Fbo.getTexture(), field1MultiplierController.value, field1BiasParameter,
                       field2Fbo.getTexture(), field2MultiplierController.value, field2BiasParameter,
                       gridParameters);
  } else if (field1Fbo.isAllocated()) {
    smearShader.render(*fboPtr, translation, mixNew, alphaMultiplier, field1Fbo.getTexture(), field1MultiplierController.value, field1BiasParameter);
  } else {
    smearShader.render(*fboPtr, translation, mixNew, alphaMultiplier);
  }
}

void SmearMod::receive(int sinkId, const float& value) {
  switch (sinkId) {
    case SINK_FLOAT:
      mixNewController.updateAuto(value, getAgency());
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


void SmearMod::applyIntent(const Intent& intent, float strength) {
  float e = intent.getEnergy();
  float d = intent.getDensity();
  float s = intent.getStructure();
  float c = intent.getChaos();
  float g = intent.getGranularity();

  mixNewController.updateIntent(inverseMap(e, mixNewController), strength);
  alphaMultiplierController.updateIntent(exponentialMap(d, alphaMultiplierController), strength);
  field1MultiplierController.updateIntent(linearMap(e, field1MultiplierController), strength);
  field2MultiplierController.updateIntent(exponentialMap(c, field2MultiplierController, 2.0f), strength);
  jumpAmountController.updateIntent(exponentialMap(c, jumpAmountController, 2.0f), strength);
  borderWidthController.updateIntent(linearMap(s, borderWidthController), strength);
  ghostBlendController.updateIntent(linearMap(d, ghostBlendController), strength);

  // TODO: what's this doing? need linearMap<int>?
  if (strength > 0.01f) {
    int levels = 1 + static_cast<int>(linearMap(s, 0.0f, 4.0f));
    gridLevelsParameter.set(levels);
    float p = linearMap(g, 4.0f, 32.0f);
    foldPeriodParameter.set(glm::vec2 { p, p });
  }
}



} // ofxMarkSynth
