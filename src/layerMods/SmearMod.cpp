//
//  SmearMod.cpp
//  fingerprint-ipad-1
//
//  Created by Steve Meyfroidt on 01/08/2025.
//

#include "SmearMod.hpp"
#include "core/IntentMapping.hpp"
#include "core/IntentMapper.hpp"
#include <cmath>



namespace ofxMarkSynth {



SmearMod::SmearMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config)
: Mod { synthPtr, name, std::move(config) }
{
  smearShader.load();

  sinkNameIdMap = {
    { translateByParameter.getName(), SINK_VEC2 },
    { mixNewParameter.getName(), SINK_FLOAT },
    { alphaMultiplierParameter.getName(), SINK_ALPHA_MULTIPLIER },
    { field1MultiplierParameter.getName(), SINK_FIELD1_MULTIPLIER },
    { field2MultiplierParameter.getName(), SINK_FIELD2_MULTIPLIER },
    { "Field1Texture", SINK_FIELD_1_TEX },
    { "Field2Texture", SINK_FIELD_2_TEX },
    { "ChangeLayer", SINK_CHANGE_LAYER }
  };

  registerControllerForSource(mixNewParameter, mixNewController);
  registerControllerForSource(alphaMultiplierParameter, alphaMultiplierController);
  registerControllerForSource(field1MultiplierParameter, field1MultiplierController);
  registerControllerForSource(field2MultiplierParameter, field2MultiplierController);
  registerControllerForSource(gridSizeParameter, gridSizeController);
  registerControllerForSource(jumpAmountParameter, jumpAmountController);
  registerControllerForSource(borderWidthParameter, borderWidthController);
  registerControllerForSource(gridLevelsParameter, gridLevelsController);
  registerControllerForSource(ghostBlendParameter, ghostBlendController);
  registerControllerForSource(foldPeriodParameter, foldPeriodController);
}

void SmearMod::initParameters() {
  parameters.add(mixNewParameter);
  parameters.add(alphaMultiplierParameter);
  parameters.add(translateByParameter);
  parameters.add(field1PreScaleExpParameter);
  parameters.add(field1MultiplierParameter);
  parameters.add(field1BiasParameter);
  parameters.add(field2PreScaleExpParameter);
  parameters.add(field2MultiplierParameter);
  parameters.add(field2BiasParameter);

  parameters.add(gridSizeParameter);
  parameters.add(strategyParameter);
  parameters.add(jumpAmountParameter);
  parameters.add(borderWidthParameter);
  parameters.add(gridLevelsParameter);
  parameters.add(ghostBlendParameter);
  parameters.add(foldPeriodParameter);
  parameters.add(agencyFactorParameter);
}

float SmearMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

void SmearMod::update() {
  syncControllerAgencies();
  mixNewController.update();
  alphaMultiplierController.update();
  field1MultiplierController.update();
  field2MultiplierController.update();
  gridSizeController.update();
  jumpAmountController.update();
  borderWidthController.update();
  gridLevelsController.update();
  ghostBlendController.update();
  foldPeriodController.update();

  auto drawingLayerPtrOpt = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) return;
  auto fboPtr = drawingLayerPtrOpt.value()->fboPtr;

  glm::vec2 translation { translateByParameter->x, translateByParameter->y };
  float mixNew = mixNewController.value;
  float alphaMultiplier = alphaMultiplierController.value;
  SmearShader::GridParameters gridParameters = {
    .gridSize = gridSizeController.value,
    .strategy = strategyParameter.get(),
    .jumpAmount = jumpAmountController.value,
    .borderWidth = borderWidthController.value,
    .gridLevels = gridLevelsController.value,
    .ghostBlend = ghostBlendController.value,
    .foldPeriod = foldPeriodController.value
  };
  ofPushStyle();
  // Shader already mixes history; disable GL blending to avoid interference
  ofEnableBlendMode(OF_BLENDMODE_DISABLED);
  // TODO: make this more forgiving
  // PreScaleExp is log10 exponent: 10^exp gives preScale to normalize field magnitude
  float field1PreScale = std::pow(10.0f, field1PreScaleExpParameter.get());
  float field2PreScale = std::pow(10.0f, field2PreScaleExpParameter.get());
  float field1EffectiveMultiplier = field1PreScale * field1MultiplierController.value;
  float field2EffectiveMultiplier = field2PreScale * field2MultiplierController.value;
  if (field2Tex.isAllocated() && field1Tex.isAllocated()) {
    smearShader.render(*fboPtr,
                       translation,
                       mixNew,
                       alphaMultiplier,
                       field1Tex,
                       field1EffectiveMultiplier,
                       field1BiasParameter,
                       field2Tex,
                       field2EffectiveMultiplier,
                       field2BiasParameter,
                       gridParameters);
  } else if (field1Tex.isAllocated()) {
    smearShader.render(*fboPtr,
                       translation,
                       mixNew,
                       alphaMultiplier,
                       field1Tex,
                       field1EffectiveMultiplier,
                       field1BiasParameter,
                       gridParameters);
  } else {
    smearShader.render(*fboPtr, translation, mixNew, alphaMultiplier, gridParameters);
  }
  ofPopStyle();
}

void SmearMod::receive(int sinkId, const float& value) {
  // Allow ChangeLayer even when inactive so the Mod can recover from disableDrawingLayer().
  if (sinkId != SINK_CHANGE_LAYER && !canDrawOnNamedLayer()) return;

  switch (sinkId) {
    case SINK_FLOAT:
      mixNewController.updateAuto(value, getAgency());
      break;
    case SINK_ALPHA_MULTIPLIER:
      alphaMultiplierController.updateAuto(value, getAgency());
      break;
    case SINK_FIELD1_MULTIPLIER:
      field1MultiplierController.updateAuto(value, getAgency());
      break;
    case SINK_FIELD2_MULTIPLIER:
      field2MultiplierController.updateAuto(value, getAgency());
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
  if (!canDrawOnNamedLayer()) return;

  switch (sinkId) {
    case SINK_VEC2:
      translateByParameter = v;
      break;
    default:
      ofLogError("SmearMod") << "glm::vec2 receive for unknown sinkId " << sinkId;
  }
}

void SmearMod::receive(int sinkId, const ofTexture& value) {
  if (!canDrawOnNamedLayer()) return;

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
  IntentMap im(intent);

  im.E().exp(mixNewController, strength);
  im.D().exp(alphaMultiplierController, strength);
  im.E().exp(field1MultiplierController, strength, 2.0f);
  im.C().exp(field2MultiplierController, strength, 3.0f);
  im.C().exp(jumpAmountController, strength, 2.0f);
  im.G().lin(borderWidthController, strength);
  im.D().lin(ghostBlendController, strength);

  if (strength > 0.05f) {
    float s = im.S().get();
    float g = im.G().get();

    int levels = 1 + static_cast<int>(linearMap(s, 0.0f, 4.0f));
    gridLevelsController.updateIntent(levels, strength, "S -> levels");

    float gridVal = linearMap(1.0f - g, 8.0f, 64.0f);
    gridSizeController.updateIntent(glm::vec2 { gridVal, gridVal }, strength, "G -> gridSize");

    float p = linearMap(g, 4.0f, 32.0f);
    foldPeriodController.updateIntent(glm::vec2 { p, p }, strength, "G -> foldPeriod");
  }
}



} // ofxMarkSynth
