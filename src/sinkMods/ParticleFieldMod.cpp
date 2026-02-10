//  ParticleFieldMod.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 10/09/2025.
//

#include "ParticleFieldMod.hpp"
#include "cmath"
#include "config/Parameter.hpp"
#include "core/IntentMapper.hpp"
#include "core/IntentMapping.hpp"

namespace ofxMarkSynth {

ParticleFieldMod::ParticleFieldMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config,
                                   float field1ValueOffset_, float field2ValueOffset_)
: Mod { synthPtr, name, std::move(config) }
{
  particleField.setup(ofFloatColor(1.0, 1.0, 1.0, 0.3), field1ValueOffset_, field2ValueOffset_);

  sinkNameIdMap = {
    { "Field1Texture", SINK_FIELD_1_FBO },
    { "Field2Texture", SINK_FIELD_2_FBO },
    { "ColourFieldTexture", SINK_COLOR_FIELD_FBO },
    { pointColorParameter.getName(), SINK_POINT_COLOR },
    { "minWeight", SINK_MIN_WEIGHT },
    { "maxWeight", SINK_MAX_WEIGHT },
    { "ChangeLayer", Mod::SINK_CHANGE_LAYER },
    { "ChangeKeyColour", SINK_CHANGE_KEY_COLOUR }
  };
}

void ParticleFieldMod::initParameters() {
  addFlattenedParameterGroup(parameters, particleField.getParameterGroup());
  parameters.add(pointColorParameter);
  parameters.add(keyColoursParameter);
  parameters.add(field1PreScaleExpParameter);
  parameters.add(field2PreScaleExpParameter);
  parameters.add(agencyFactorParameter);

  minWeightControllerPtr = std::make_unique<ParamController<float>>(parameters.get("minWeight").cast<float>());
  maxWeightControllerPtr = std::make_unique<ParamController<float>>(parameters.get("maxWeight").cast<float>());
  pointColorControllerPtr = std::make_unique<ParamController<ofFloatColor>>(pointColorParameter);

  ln2ParticleCountControllerPtr = std::make_unique<ParamController<float>>(parameters.get("ln2ParticleCount").cast<float>());

  velocityDampingControllerPtr = std::make_unique<ParamController<float>>(parameters.get("velocityDamping").cast<float>());
  forceMultiplierControllerPtr = std::make_unique<ParamController<float>>(parameters.get("forceMultiplier").cast<float>());
  maxVelocityControllerPtr = std::make_unique<ParamController<float>>(parameters.get("maxVelocity").cast<float>());
  particleSizeControllerPtr = std::make_unique<ParamController<float>>(parameters.get("particleSize").cast<float>());
  jitterStrengthControllerPtr = std::make_unique<ParamController<float>>(parameters.get("jitterStrength").cast<float>());
  jitterSmoothingControllerPtr = std::make_unique<ParamController<float>>(parameters.get("jitterSmoothing").cast<float>());
  speedThresholdControllerPtr = std::make_unique<ParamController<float>>(parameters.get("speedThreshold").cast<float>());
  field1MultiplierControllerPtr = std::make_unique<ParamController<float>>(parameters.get("field1Multiplier").cast<float>());
  field2MultiplierControllerPtr = std::make_unique<ParamController<float>>(parameters.get("field2Multiplier").cast<float>());

  auto& minWeightParam = parameters.get("minWeight").cast<float>();
  auto& maxWeightParam = parameters.get("maxWeight").cast<float>();
  registerControllerForSource(minWeightParam, *minWeightControllerPtr);
  registerControllerForSource(maxWeightParam, *maxWeightControllerPtr);
  registerControllerForSource(pointColorParameter, *pointColorControllerPtr);

  registerControllerForSource(parameters.get("ln2ParticleCount").cast<float>(), *ln2ParticleCountControllerPtr);
  registerControllerForSource(parameters.get("velocityDamping").cast<float>(), *velocityDampingControllerPtr);
  registerControllerForSource(parameters.get("forceMultiplier").cast<float>(), *forceMultiplierControllerPtr);
  registerControllerForSource(parameters.get("maxVelocity").cast<float>(), *maxVelocityControllerPtr);
  registerControllerForSource(parameters.get("particleSize").cast<float>(), *particleSizeControllerPtr);
  registerControllerForSource(parameters.get("jitterStrength").cast<float>(), *jitterStrengthControllerPtr);
  registerControllerForSource(parameters.get("jitterSmoothing").cast<float>(), *jitterSmoothingControllerPtr);
  registerControllerForSource(parameters.get("speedThreshold").cast<float>(), *speedThresholdControllerPtr);
  registerControllerForSource(parameters.get("field1Multiplier").cast<float>(), *field1MultiplierControllerPtr);
  registerControllerForSource(parameters.get("field2Multiplier").cast<float>(), *field2MultiplierControllerPtr);
}

float ParticleFieldMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

void ParticleFieldMod::update() {
  syncControllerAgencies();
  if (minWeightControllerPtr) minWeightControllerPtr->update();
  if (maxWeightControllerPtr) maxWeightControllerPtr->update();
  if (pointColorControllerPtr) pointColorControllerPtr->update();
  if (ln2ParticleCountControllerPtr) ln2ParticleCountControllerPtr->update();
  if (velocityDampingControllerPtr) velocityDampingControllerPtr->update();
  if (forceMultiplierControllerPtr) forceMultiplierControllerPtr->update();
  if (maxVelocityControllerPtr) maxVelocityControllerPtr->update();
  if (particleSizeControllerPtr) particleSizeControllerPtr->update();
  if (jitterStrengthControllerPtr) jitterStrengthControllerPtr->update();
  if (jitterSmoothingControllerPtr) jitterSmoothingControllerPtr->update();
  if (speedThresholdControllerPtr) speedThresholdControllerPtr->update();
  if (field1MultiplierControllerPtr) field1MultiplierControllerPtr->update();
  if (field2MultiplierControllerPtr) field2MultiplierControllerPtr->update();

  auto drawingLayerPtrOpt = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) return;
  auto drawingLayerPtr = drawingLayerPtrOpt.value();
  auto fboPtr = drawingLayerPtr->fboPtr;

  // Active recoloring based on blended PointColour value
  if (pointColorControllerPtr && particleField.getParticleCount() > 0) {
    auto particleBlocks = particleField.getParticleCount() / 64;
    auto baseBlocks = std::max(1, std::min(particleBlocks / 100, 64));
    float agency = getAgency();
    if (agency < 0.0f) agency = 0.0f;
    if (agency > 1.0f) agency = 1.0f;
    auto updateBlocks = std::max(1, static_cast<int>(baseBlocks * agency));
    ofFloatColor color = pointColorControllerPtr->value;
    particleField.updateRandomColorBlocks(updateBlocks, 64, [&color](size_t) {
      return color;
    });
  }

  {
    ofxParticleField::ParticleField::ParameterOverrides overrides;
    overrides.velocityDamping = std::clamp(velocityDampingControllerPtr->value,
                                          particleField.velocityDampingParameter.getMin(),
                                          particleField.velocityDampingParameter.getMax());
    overrides.forceMultiplier = std::clamp(forceMultiplierControllerPtr->value,
                                          particleField.forceMultiplierParameter.getMin(),
                                          particleField.forceMultiplierParameter.getMax());
    overrides.maxVelocity = std::clamp(maxVelocityControllerPtr->value,
                                       particleField.maxVelocityParameter.getMin(),
                                       particleField.maxVelocityParameter.getMax());
    overrides.particleSize = std::clamp(particleSizeControllerPtr->value,
                                        particleField.particleSizeParameter.getMin(),
                                        particleField.particleSizeParameter.getMax());
    overrides.jitterStrength = std::clamp(jitterStrengthControllerPtr->value,
                                         particleField.jitterStrengthParameter.getMin(),
                                         particleField.jitterStrengthParameter.getMax());
    overrides.jitterSmoothing = std::clamp(jitterSmoothingControllerPtr->value,
                                          particleField.jitterSmoothingParameter.getMin(),
                                          particleField.jitterSmoothingParameter.getMax());
    overrides.speedThreshold = std::clamp(speedThresholdControllerPtr->value,
                                         particleField.speedThresholdParameter.getMin(),
                                         particleField.speedThresholdParameter.getMax());
    overrides.minWeight = std::clamp(minWeightControllerPtr->value,
                                     particleField.minWeightParameter.getMin(),
                                     particleField.minWeightParameter.getMax());
    overrides.maxWeight = std::clamp(maxWeightControllerPtr->value,
                                     particleField.maxWeightParameter.getMin(),
                                     particleField.maxWeightParameter.getMax());

    const float field1PreScale = std::pow(10.0f, field1PreScaleExpParameter.get());
    const float field2PreScale = std::pow(10.0f, field2PreScaleExpParameter.get());
    const float field1MultEffective = field1MultiplierControllerPtr->value * field1PreScale;
    const float field2MultEffective = field2MultiplierControllerPtr->value * field2PreScale;
    // Allow effective multipliers beyond the underlying parameter range: preScaleExp is a normalization stage.
    overrides.field1Multiplier = std::clamp(field1MultEffective, 0.0f, 200.0f);
    overrides.field2Multiplier = std::clamp(field2MultEffective, 0.0f, 200.0f);

    const bool overridesChanged = !hasLastAppliedParameterOverrides
                                 || overrides.velocityDamping != lastAppliedParameterOverrides.velocityDamping
                                 || overrides.forceMultiplier != lastAppliedParameterOverrides.forceMultiplier
                                 || overrides.maxVelocity != lastAppliedParameterOverrides.maxVelocity
                                 || overrides.particleSize != lastAppliedParameterOverrides.particleSize
                                 || overrides.jitterStrength != lastAppliedParameterOverrides.jitterStrength
                                 || overrides.jitterSmoothing != lastAppliedParameterOverrides.jitterSmoothing
                                 || overrides.speedThreshold != lastAppliedParameterOverrides.speedThreshold
                                 || overrides.minWeight != lastAppliedParameterOverrides.minWeight
                                 || overrides.maxWeight != lastAppliedParameterOverrides.maxWeight
                                 || overrides.field1Multiplier != lastAppliedParameterOverrides.field1Multiplier
                                 || overrides.field2Multiplier != lastAppliedParameterOverrides.field2Multiplier;

    if (overridesChanged) {
      particleField.setParameterOverrides(overrides);
      lastAppliedParameterOverrides = overrides;
      hasLastAppliedParameterOverrides = true;
    }
  }

  particleField.update();

//  // Use ALPHA for layers that clear on update, else SCREEN
//  if (drawingLayerPtr->clearOnUpdate) ofEnableBlendMode(OF_BLENDMODE_SCREEN);
//  else ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  ofPushStyle();
  ofEnableBlendMode(OF_BLENDMODE_SCREEN);

//  particleField.draw(fboPtr->getSource(), !drawingLayerPtr->clearOnUpdate);
  particleField.draw(fboPtr->getSource());
  ofPopStyle();
}

void ParticleFieldMod::receive(int sinkId, const ofTexture& value) {
  if (!canDrawOnNamedLayer()) return;

  switch (sinkId) {
    case SINK_FIELD_1_FBO:
      particleField.setField1(value);
      break;
    case SINK_FIELD_2_FBO:
      particleField.setField2(value);
      break;
    default:
      ofLogError("ParticleFieldMod") << "ofFbo receive for unknown sinkId " << sinkId;
  }
}

void ParticleFieldMod::receive(int sinkId, const glm::vec4& v) {
  if (!canDrawOnNamedLayer()) return;

  switch (sinkId) {
    case SINK_POINT_COLOR: {
      if (pointColorControllerPtr) {
        pointColorControllerPtr->updateAuto(ofFloatColor { v.r, v.g, v.b, v.a }, getAgency());
      }
      break;
    }
    default:
      ofLogError("ParticleFieldMod") << "glm::vec4 receive for unknown sinkId " << sinkId;
  }
}

void ParticleFieldMod::receive(int sinkId, const float& value) {
  if (sinkId != SINK_CHANGE_KEY_COLOUR && !canDrawOnNamedLayer()) return;

  switch (sinkId) {
    case SINK_CHANGE_KEY_COLOUR:
      if (value > 0.5f) {
        // Key colour flip is independent of agency; agency affects how auto colour mixes in.
        keyColourRegister.ensureInitialized(keyColourRegisterInitialized, keyColoursParameter.get(), pointColorParameter.get());
        keyColourRegister.flip();
        pointColorParameter.set(keyColourRegister.getCurrentColour());
      }
      break;

    case Mod::SINK_CHANGE_LAYER:
      if (value > 0.5f) {
        ofLogNotice("ParticleFieldMod") << "ParticleFieldMod::ChangeLayer: changing layer";
        changeDrawingLayer();
      }
      break;
    case SINK_MIN_WEIGHT:
      minWeightControllerPtr->updateAuto(value, getAgency());
      break;
    case SINK_MAX_WEIGHT:
      maxWeightControllerPtr->updateAuto(value, getAgency());
      break;
    default:
      ofLogError("ParticleFieldMod") << "Float receive for unknown sinkId " << sinkId;
  }
}

void ParticleFieldMod::applyIntent(const Intent& intent, float strength) {
  IntentMap im(intent);

  // Color composition feeds PointColour controller as intent contribution
  ofFloatColor intentColor = ofxMarkSynth::energyToColor(intent);
  intentColor.setBrightness(ofxMarkSynth::structureToBrightness(intent) * 0.5f);
  intentColor.setSaturation(im.E().get() * im.C().get());
  intentColor.a = ofLerp(0.1f, 0.5f, im.D().get());
  if (pointColorControllerPtr) {
    pointColorControllerPtr->updateIntent(intentColor, strength, "E,S,C,D → PointColour");
  }

  // Density controls particle count, but keep it near the tuned baseline.
  im.D().expAround(*ln2ParticleCountControllerPtr,
                    strength,
                    3.0f,
                    Mapping::WithFractions{0.20f, 0.20f});

  // Weight: higher granularity tends to clump more, so increase weight (inertia) as G rises.
  im.G().linAround(*minWeightControllerPtr, strength);
  im.C().linAround(*maxWeightControllerPtr, strength);

  // Physics parameters
  im.G().inv().linAround(*velocityDampingControllerPtr, strength);
  im.E().expAround(*forceMultiplierControllerPtr, strength, 4.0f);
  im.E().expAround(*maxVelocityControllerPtr, strength, 4.0f);

  // Visual parameters
  // Granularity affects feature size, but with heavy damping.
  im.G().expAround(*particleSizeControllerPtr, strength, 5.0f);

  // Jitter parameters
  // Keep particles mostly field-driven even at high Chaos; jitter is mainly an escape from clumping.
  im.C().expAround(*jitterStrengthControllerPtr, strength, 5.0f);
  im.S().linAround(*jitterSmoothingControllerPtr, strength);

  // Speed threshold
  (im.E() * im.C()).linAround(*speedThresholdControllerPtr, strength);

  // Field influence
  // Dampen high-intent extremes: fields should remain coherent rather than becoming "teleporty".
  im.E().expAround(*field1MultiplierControllerPtr, strength, 3.0f);
  im.C().expAround(*field2MultiplierControllerPtr, strength, 4.0f);
}

} // ofxMarkSynth
