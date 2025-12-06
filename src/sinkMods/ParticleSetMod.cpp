//
//  ParticleSetMod.cpp
//  example_particles
//
//  Created by Steve Meyfroidt on 09/05/2025.
//

#include "ParticleSetMod.hpp"
#include "IntentMapping.hpp"
#include "Parameter.hpp"
#include "../IntentMapper.hpp"


namespace ofxMarkSynth {



ParticleSetMod::ParticleSetMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config)
: Mod { synthPtr, name, std::move(config) }
{
  sinkNameIdMap = {
    { "Point", SINK_POINT },
    { "PointVelocity", SINK_POINT_VELOCITY },
    { spinParameter.getName(), SINK_SPIN },
    { colorParameter.getName(), SINK_COLOR }
  };
  
  registerControllerForSource(spinParameter, spinController);
  registerControllerForSource(colorParameter, colorController);
}

float ParticleSetMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

void ParticleSetMod::initParameters() {
  parameters.add(spinParameter);
  parameters.add(colorParameter);
  addFlattenedParameterGroup(parameters, particleSet.getParameterGroup());
  parameters.add(agencyFactorParameter);

  timeStepControllerPtr = std::make_unique<ParamController<float>>(parameters.get("timeStep").cast<float>());
  velocityDampingControllerPtr = std::make_unique<ParamController<float>>(parameters.get("velocityDamping").cast<float>());
  attractionStrengthControllerPtr = std::make_unique<ParamController<float>>(parameters.get("attractionStrength").cast<float>());
  attractionRadiusControllerPtr = std::make_unique<ParamController<float>>(parameters.get("attractionRadius").cast<float>());
  forceScaleControllerPtr = std::make_unique<ParamController<float>>(parameters.get("forceScale").cast<float>());
  connectionRadiusControllerPtr = std::make_unique<ParamController<float>>(parameters.get("connectionRadius").cast<float>());
  colourMultiplierControllerPtr = std::make_unique<ParamController<float>>(parameters.get("colourMultiplier").cast<float>());
  maxSpeedControllerPtr = std::make_unique<ParamController<float>>(parameters.get("maxSpeed").cast<float>());
  
  auto& timeStepParam = parameters.get("timeStep").cast<float>();
  auto& velocityDampingParam = parameters.get("velocityDamping").cast<float>();
  auto& attractionStrengthParam = parameters.get("attractionStrength").cast<float>();
  auto& attractionRadiusParam = parameters.get("attractionRadius").cast<float>();
  auto& forceScaleParam = parameters.get("forceScale").cast<float>();
  auto& connectionRadiusParam = parameters.get("connectionRadius").cast<float>();
  auto& colourMultiplierParam = parameters.get("colourMultiplier").cast<float>();
  auto& maxSpeedParam = parameters.get("maxSpeed").cast<float>();
  registerControllerForSource(timeStepParam, *timeStepControllerPtr);
  registerControllerForSource(velocityDampingParam, *velocityDampingControllerPtr);
  registerControllerForSource(attractionStrengthParam, *attractionStrengthControllerPtr);
  registerControllerForSource(attractionRadiusParam, *attractionRadiusControllerPtr);
  registerControllerForSource(forceScaleParam, *forceScaleControllerPtr);
  registerControllerForSource(connectionRadiusParam, *connectionRadiusControllerPtr);
  registerControllerForSource(colourMultiplierParam, *colourMultiplierControllerPtr);
  registerControllerForSource(maxSpeedParam, *maxSpeedControllerPtr);
}

void ParticleSetMod::update() {
  syncControllerAgencies();
  spinController.update();
  colorController.update();
  timeStepControllerPtr->update();
  velocityDampingControllerPtr->update();
  attractionStrengthControllerPtr->update();
  attractionRadiusControllerPtr->update();
  forceScaleControllerPtr->update();
  connectionRadiusControllerPtr->update();
  colourMultiplierControllerPtr->update();
  maxSpeedControllerPtr->update();
  
  auto drawingLayerPtrOpt = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) return;
  auto fboPtr = drawingLayerPtrOpt.value()->fboPtr;

  particleSet.update();
  
  std::for_each(newPoints.begin(), newPoints.end(), [this](const auto& vec) {
    glm::vec2 p { vec.x, vec.y };
    glm::vec2 v { vec.z, vec.w };
    particleSet.add(p, v, colorController.value, spinController.value);
  });
  newPoints.clear();
  
  fboPtr->getSource().begin();
  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
//  ofScale(fboPtr->getWidth(), fboPtr->getHeight());
  particleSet.draw(glm::vec2(fboPtr->getWidth(), fboPtr->getHeight()));
  fboPtr->getSource().end();
}

void ParticleSetMod::receive(int sinkId, const float& value) {
  switch (sinkId) {
    case SINK_SPIN:
      spinController.updateAuto(value, getAgency());
      break;
    default:
      ofLogError("ParticleSetMod") << "Float receive for unknown sinkId " << sinkId;
  }
}

void ParticleSetMod::receive(int sinkId, const glm::vec2& point) {
  switch (sinkId) {
    case SINK_POINT:
      newPoints.push_back(glm::vec4 { point, ofRandom(0.01)-0.005, ofRandom(0.01)-0.005 });
      break;
    default:
      ofLogError("ParticleSetMod") << "glm::vec2 receive for unknown sinkId " << sinkId;
  }
}

void ParticleSetMod::receive(int sinkId, const glm::vec4& v) {
  switch (sinkId) {
    case SINK_POINT_VELOCITY:
      newPoints.push_back(v);
      break;
    case SINK_COLOR:
      colorController.updateAuto(ofFloatColor { v.r, v.g, v.b, v.a }, getAgency());
      break;
    default:
      ofLogError("ParticleSetMod") << "glm::vec4 receive for unknown sinkId " << sinkId;
  }
}

void ParticleSetMod::applyIntent(const Intent& intent, float strength) {
  IntentMap im(intent);
  
  (im.C() * im.E()).lin(spinController, strength, -0.05f, 0.05f);
  
  // Color composition
  ofFloatColor color = ofxMarkSynth::energyToColor(intent);
  color.setBrightness(ofxMarkSynth::structureToBrightness(intent));
  color.setSaturation((im.E() * im.C() * im.S().inv()).get());
  color.a = im.D().get();
  colorController.updateIntent(color, strength, "E->color, S->bright, E*C*(1-S)->sat, D->alpha");
  
  im.E().exp(*timeStepControllerPtr, strength);
  im.G().inv().lin(*velocityDampingControllerPtr, strength);
  im.S().lin(*attractionStrengthControllerPtr, strength);
  im.D().inv().lin(*attractionRadiusControllerPtr, strength);
  im.E().lin(*forceScaleControllerPtr, strength);
  im.D().lin(*connectionRadiusControllerPtr, strength);
  im.E().lin(*colourMultiplierControllerPtr, strength);
  im.C().lin(*maxSpeedControllerPtr, strength);
}


} // ofxMarkSynth
