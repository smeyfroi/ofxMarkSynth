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
  
  sourceNameControllerPtrMap = {
    { mixNewParameter.getName(), &mixNewController },
    { alphaMultiplierParameter.getName(), &alphaMultiplierController },
    { field1MultiplierParameter.getName(), &field1MultiplierController },
    { field2MultiplierParameter.getName(), &field2MultiplierController },
    { jumpAmountParameter.getName(), &jumpAmountController },
    { borderWidthParameter.getName(), &borderWidthController },
    { ghostBlendParameter.getName(), &ghostBlendController }
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
        ofLogNotice("SmearMod") << "SmearMod::SINK_CHANGE_LAYER: disable layer";
        disableDrawingLayer();
      } else if (value > 0.6) { // FIXME: temp until connections have weights
        ofLogNotice("SmearMod") << "SmearMod::SINK_CHANGE_LAYER: changing layer";
        changeDrawingLayer();
      } else if (value > 0.3) {
        // higher chance to return to default layer
        ofLogNotice("SmearMod") << "SmearMod::SINK_CHANGE_LAYER: default layer";
        resetDrawingLayer();
      }
      break;
    default:
      ofLogError("SmearMod") << "Float receive for unknown sinkId " << sinkId;
  }
}

void SmearMod::receive(int sinkId, const glm::vec2& v) {
  switch (sinkId) {
    case SINK_VEC2:
      translateByParameter = v;
      break;
    default:
      ofLogError("SmearMod") << "glm::vec2 receive for unknown sinkId " << sinkId;
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
      ofLogError("SmearMod") << "ofFbo receive for unknown sinkId " << sinkId;
  }
}


void SmearMod::applyIntent(const Intent& intent, float strength) {
  if (strength < 0.01) return;

  float e = intent.getEnergy();
  float d = intent.getDensity();
  float s = intent.getStructure();
  float c = intent.getChaos();
  float g = intent.getGranularity();

  // Energy -> MixNew
  mixNewController.updateIntent(exponentialMap(e, mixNewController), strength);
  // Density -> AlphaMultiplier
  alphaMultiplierController.updateIntent(exponentialMap(d, alphaMultiplierController), strength);
  // Energy -> Field multiplier 1
  field1MultiplierController.updateIntent(exponentialMap(e, field1MultiplierController, 2.0f), strength);
  // Chaos -> Field multiplier 2
  field2MultiplierController.updateIntent(exponentialMap(c, field2MultiplierController, 3.0f), strength);
  // Chaos -> JumpAmount
  jumpAmountController.updateIntent(exponentialMap(c, jumpAmountController, 2.0f), strength);
  // Granularity -> BorderWidth
  borderWidthController.updateIntent(linearMap(g, borderWidthController), strength);
  // Density -> GhostBlend
  ghostBlendController.updateIntent(linearMap(d, ghostBlendController), strength);

  // TODO: fix these
//  if (strength > 0.01f) {
//    int levels = 1 + static_cast<int>(linearMap(s, 0.0f, 4.0f));
//    gridLevelsParameter.set(levels);
//    float p = linearMap(g, 4.0f, 32.0f);
//    foldPeriodParameter.set(glm::vec2 { p, p });
//  }
}



} // ofxMarkSynth
