//
//  FluidRadialImpulseMod.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 25/05/2025.
//

#include "FluidRadialImpulseMod.hpp"

#include <algorithm>

#include "core/IntentMapping.hpp"
#include "core/IntentMapper.hpp"



namespace ofxMarkSynth {



FluidRadialImpulseMod::FluidRadialImpulseMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config)
: Mod { synthPtr, name, std::move(config) }
{
  addRadialImpulseShader.load();

  sinkNameIdMap = {
    { "Point", SINK_POINTS },
    { "PointVelocity", SINK_POINT_VELOCITY },
    { "Velocity", SINK_VELOCITY },
    { "SwirlVelocity", SINK_SWIRL_VELOCITY },
    { impulseRadiusParameter.getName(), SINK_IMPULSE_RADIUS },
    { impulseStrengthParameter.getName(), SINK_IMPULSE_STRENGTH }
  };

  registerControllerForSource(impulseRadiusParameter, impulseRadiusController);
  registerControllerForSource(impulseStrengthParameter, impulseStrengthController);
  registerControllerForSource(swirlStrengthParameter, swirlStrengthController);
  registerControllerForSource(swirlVelocityParameter, swirlVelocityController);
}

void FluidRadialImpulseMod::initParameters() {
  parameters.add(impulseRadiusParameter);
  parameters.add(impulseStrengthParameter);
  parameters.add(dtParameter);
  parameters.add(velocityScaleParameter);
  parameters.add(swirlStrengthParameter);
  parameters.add(swirlVelocityParameter);
  parameters.add(agencyFactorParameter);
}

float FluidRadialImpulseMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

void FluidRadialImpulseMod::update() {
  syncControllerAgencies();
  impulseRadiusController.update();
  impulseStrengthController.update();
  swirlStrengthController.update();
  swirlVelocityController.update();

  auto drawingLayerPtrOpt = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) {
    newPoints.clear();
    newPointVelocities.clear();
    return;
  }
  auto fboPtr = drawingLayerPtrOpt.value()->fboPtr;

  const float dt = dtParameter.get();
  const float w = fboPtr->getWidth();
  const float h = fboPtr->getHeight();
  const float minDim = std::min(w, h);
  const float radiusPx = impulseRadiusController.value * minDim;

  // Interpret strength as a fraction of radius displacement per step.
  const float radialVelocityPx = impulseStrengthController.value * radiusPx;

  const float swirlNorm = std::clamp(swirlVelocityController.value * swirlStrengthController.value, 0.0f, 1.0f);
  const float swirlVelocityPx = swirlNorm * radiusPx;

  const float velScale = velocityScaleParameter.get();

  // Point-only: use the current global velocity (if any).
  const glm::vec2 globalVelocityPx = velScale * glm::vec2 { currentVelocityNorm.x * w, currentVelocityNorm.y * h };

  std::for_each(newPoints.begin(), newPoints.end(), [this, dt, fboPtr, radiusPx, globalVelocityPx, radialVelocityPx, swirlVelocityPx](const auto& p) {
    const glm::vec2 centerPx { p.x * fboPtr->getWidth(), p.y * fboPtr->getHeight() };
    addRadialImpulseShader.render(*fboPtr,
                                  centerPx,
                                  radiusPx,
                                  globalVelocityPx,
                                  radialVelocityPx,
                                  swirlVelocityPx,
                                  dt);
  });

  // PointVelocity: use per-point velocity plus radial + swirl.
  std::for_each(newPointVelocities.begin(), newPointVelocities.end(), [this, dt, fboPtr, radiusPx, velScale, radialVelocityPx, swirlVelocityPx](const auto& pv) {
    const float w = fboPtr->getWidth();
    const float h = fboPtr->getHeight();
    const glm::vec2 centerPx { pv.x * w, pv.y * h };
    const glm::vec2 velocityPx = velScale * glm::vec2 { pv.z * w, pv.w * h };
    addRadialImpulseShader.render(*fboPtr,
                                  centerPx,
                                  radiusPx,
                                  velocityPx,
                                  radialVelocityPx,
                                  swirlVelocityPx,
                                  dt);
  });

  newPoints.clear();
  newPointVelocities.clear();
}

void FluidRadialImpulseMod::receive(int sinkId, const float& value) {
  if (!canDrawOnNamedLayer()) return;

  switch (sinkId) {
    case SINK_IMPULSE_RADIUS:
      impulseRadiusController.updateAuto(value, getAgency());
      break;
    case SINK_IMPULSE_STRENGTH:
      impulseStrengthController.updateAuto(value, getAgency());
      break;
    case SINK_SWIRL_VELOCITY:
      swirlVelocityController.updateAuto(std::clamp(value, 0.0f, 1.0f), getAgency());
      break;
    default:
      ofLogError("FluidRadialImpulseMod") << "Float receive for unknown sinkId " << sinkId;
  }
}

void FluidRadialImpulseMod::receive(int sinkId, const glm::vec2& point) {
  if (!canDrawOnNamedLayer()) return;

  switch (sinkId) {
    case SINK_POINTS:
      newPoints.push_back(point);
      break;
    case SINK_VELOCITY:
      currentVelocityNorm = point;
      break;
    default:
      ofLogError("FluidRadialImpulseMod") << "glm::vec2 receive for unknown sinkId " << sinkId;
  }
}

void FluidRadialImpulseMod::receive(int sinkId, const glm::vec4& pointVelocity) {
  if (!canDrawOnNamedLayer()) return;

  switch (sinkId) {
    case SINK_POINT_VELOCITY:
      newPointVelocities.push_back(pointVelocity);
      break;
    default:
      ofLogError("FluidRadialImpulseMod") << "glm::vec4 receive for unknown sinkId " << sinkId;
  }
}

void FluidRadialImpulseMod::applyIntent(const Intent& intent, float strength) {
  IntentMap im(intent);

  // High Structure should feel more ordered: smaller, gentler impulses.
  const float structure = im.S().get();

  // Radius is primarily Granularity-driven, then attenuated by Structure.
  float radiusBase = exponentialMap(im.G().get(),
                                   impulseRadiusController.getManualMin(),
                                   impulseRadiusController.getManualMax(),
                                   2.0f);
  float radiusScale = 1.0f - structure * 0.4f; // S=1 -> 60%
  float impulseRadiusI = std::clamp(radiusBase * radiusScale,
                                   impulseRadiusController.getManualMin(),
                                   impulseRadiusController.getManualMax());
  impulseRadiusController.updateIntent(impulseRadiusI, strength, "G -> exp; *(1-0.4*S)");

  // Strength is E/C-driven (weighted), then attenuated by Structure.
  float combinedIntentStrength = im.E().get() * 0.8f + im.C().get() * 0.2f;
  float strengthBase = exponentialMap(combinedIntentStrength,
                                     impulseStrengthController.getManualMin(),
                                     impulseStrengthController.getManualMax() * 0.5f,
                                     4.0f);
  float strengthScale = 1.0f - structure * 0.7f; // S=1 -> 30%
  float impulseStrengthI = std::clamp(strengthBase * strengthScale,
                                     impulseStrengthController.getManualMin(),
                                     impulseStrengthController.getManualMax() * 0.5f);
  impulseStrengthController.updateIntent(impulseStrengthI, strength, "E*.8+C*.2 -> exp(4)[0..0.5] * (1-0.7*S)");

  // Swirl: driven by Chaos, reduced by Structure.
  float swirlDim = im.C().get() * (1.0f - structure * 0.7f); // S=1 -> 30%
  swirlDim = std::clamp(swirlDim, 0.0f, 1.0f);

  float swirlStrengthI = exponentialMap(swirlDim,
                                       swirlStrengthController.getManualMin(),
                                       swirlStrengthController.getManualMax(),
                                       2.0f);
  swirlStrengthController.updateIntent(swirlStrengthI, strength, "C*(1-0.7*S) -> exp(2)");

  float swirlVelocityI = exponentialMap(swirlDim,
                                       swirlVelocityController.getManualMin(),
                                       swirlVelocityController.getManualMax(),
                                       2.0f);
  swirlVelocityController.updateIntent(swirlVelocityI, strength, "C*(1-0.7*S) -> exp(2)");

}
} // ofxMarkSynth
