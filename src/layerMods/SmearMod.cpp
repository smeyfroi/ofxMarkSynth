//
//  SmearMod.cpp
//  fingerprint-ipad-1
//
//  Created by Steve Meyfroidt on 01/08/2025.
//

#include "SmearMod.hpp"
#include "IntentMapping.hpp"



namespace ofxMarkSynth {



SmearMod::SmearMod(Synth* synthPtr, const std::string& name, ModConfig config)
: Mod { synthPtr, name, std::move(config) }
{
  smearShader.load();
  
  sinkNameIdMap = {
    { "Translation", SINK_VEC2 },
    { "MixNew", SINK_FLOAT },
    { "Field1Texture", SINK_FIELD_1_TEX },
    { "Field2Texture", SINK_FIELD_2_TEX },
    { "ChangeLayer", SINK_CHANGE_LAYER }
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
  syncControllerAgencies();
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
  if (field2Tex.isAllocated() && field1Tex.isAllocated()) {
    smearShader.render(*fboPtr, translation, mixNew, alphaMultiplier,
                       field1Tex, field1MultiplierController.value, field1BiasParameter,
                       field2Tex, field2MultiplierController.value, field2BiasParameter,
                       gridParameters);
  } else if (field1Tex.isAllocated()) {
    smearShader.render(*fboPtr, translation, mixNew, alphaMultiplier, field1Tex, field1MultiplierController.value, field1BiasParameter);
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

void SmearMod::receive(int sinkId, const ofTexture& value) {
  switch (sinkId) {
    case SINK_FIELD_1_TEX:
      field1Tex = value;
      break;
    case SINK_FIELD_2_TEX:
      field2Tex = value;
      break;
    default:
      ofLogError("SmearMod") << "ofTexture receive for unknown sinkId " << sinkId;
  }
}

void SmearMod::applyIntent(const Intent& intent, float strength) {

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

  // Structure → Strategy preference (more structure = more organized strategies)
  if (strength > 0.05f) {
    int strategy;
    if (s < 0.2f) {
      strategy = 0; // Off for very low structure
    } else if (s < 0.4f) {
      strategy = 2; // Per-cell random offset for low structure
    } else if (s < 0.6f) {
      strategy = 4; // Per-cell rotation for mid structure
    } else if (s < 0.8f) {
      strategy = 1; // Cell-quantized for mid-high structure
    } else {
      strategy = 3; // Boundary teleport for high structure
    }
    if (strategyParameter.get() != strategy) strategyParameter.set(strategy);
    
    // Structure → gridLevels
    int levels = 1 + static_cast<int>(linearMap(s, 0.0f, 4.0f));
    if (gridLevelsParameter.get() != levels) gridLevelsParameter.set(levels);
    
    // Granularity → gridSize (fine granularity = larger grid cells)
    float gridVal = linearMap(1.0f - g, 8.0f, 64.0f);
    gridSizeParameter.set(glm::vec2 { gridVal, gridVal });
    
    // Granularity → foldPeriod
    float p = linearMap(g, 4.0f, 32.0f);
    foldPeriodParameter.set(glm::vec2 { p, p });
  }
}



} // ofxMarkSynth
