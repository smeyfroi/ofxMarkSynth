//
//  ParticleSetMod.cpp
//  example_particles
//
//  Created by Steve Meyfroidt on 09/05/2025.
//

#include "ParticleSetMod.hpp"
#include "IntentMapping.hpp"
#include "Parameter.hpp"


namespace ofxMarkSynth {



ParticleSetMod::ParticleSetMod(Synth* synthPtr, const std::string& name, ModConfig config)
: Mod { synthPtr, name, std::move(config) }
{
  sinkNameIdMap = {
    { "points", SINK_POINTS },
    { "pointVelocities", SINK_POINT_VELOCITIES },
    { spinParameter.getName(), SINK_SPIN },
    { colorParameter.getName(), SINK_COLOR }
  };
  
  sourceNameControllerPtrMap = {
    { spinParameter.getName(), &spinController },
    { colorParameter.getName(), &colorController }
  };
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
  
  sourceNameControllerPtrMap = {
    { spinParameter.getName(), &spinController },
    { colorParameter.getName(), &colorController },
    { parameters.get("timeStep").cast<float>().getName(), timeStepControllerPtr.get() },
    { parameters.get("velocityDamping").cast<float>().getName(), velocityDampingControllerPtr.get() },
    { parameters.get("attractionStrength").cast<float>().getName(), attractionStrengthControllerPtr.get() },
    { parameters.get("attractionRadius").cast<float>().getName(), attractionRadiusControllerPtr.get() },
    { parameters.get("forceScale").cast<float>().getName(), forceScaleControllerPtr.get() },
    { parameters.get("connectionRadius").cast<float>().getName(), connectionRadiusControllerPtr.get() },
    { parameters.get("colourMultiplier").cast<float>().getName(), colourMultiplierControllerPtr.get() },
    { parameters.get("maxSpeed").cast<float>().getName(), maxSpeedControllerPtr.get() }
  };
}

void ParticleSetMod::update() {
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
    case SINK_POINTS:
      newPoints.push_back(glm::vec4 { point, ofRandom(0.01)-0.005, ofRandom(0.01)-0.005 });
      break;
    default:
      ofLogError("ParticleSetMod") << "glm::vec2 receive for unknown sinkId " << sinkId;
  }
}

void ParticleSetMod::receive(int sinkId, const glm::vec4& v) {
  switch (sinkId) {
    case SINK_POINT_VELOCITIES:
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
  if (strength < 0.01) return;
  
  // Chaos * Energy → spin (small range around 0)
  spinController.updateIntent(ofxMarkSynth::linearMap(intent.getEnergy() * intent.getChaos(), -0.05f, 0.05f), strength);
  
  // Energy → color (with structure → brightness and density → alpha)
  ofFloatColor color = ofxMarkSynth::energyToColor(intent);
  color.setBrightness(ofxMarkSynth::structureToBrightness(intent));
  color.setSaturation(intent.getEnergy() * intent.getChaos() * (1.0f - intent.getStructure()));
  color.a = ofxMarkSynth::linearMap(intent.getDensity(), 0.0f, 1.0f);
  colorController.updateIntent(color, strength);
  
  // Energy → timeStep (exponential: more energy = faster time)
  float timeStepI = exponentialMap(intent.getEnergy(), *timeStepControllerPtr);
  timeStepControllerPtr->updateIntent(timeStepI, strength);
  
  // Inverse Granularity → velocityDamping (inverse: coarse = less damping)
  float velocityDampingI = inverseMap(intent.getGranularity(), *velocityDampingControllerPtr);
  velocityDampingControllerPtr->updateIntent(velocityDampingI, strength);
  
  // Structure → attractionStrength (linear: more structure = more attraction)
  float attractionStrengthI = linearMap(intent.getStructure(), *attractionStrengthControllerPtr);
  attractionStrengthControllerPtr->updateIntent(attractionStrengthI, strength);
  
  // Density → attractionRadius (inverse: more density = smaller radius)
  float attractionRadiusI = inverseMap(intent.getDensity(), *attractionRadiusControllerPtr);
  attractionRadiusControllerPtr->updateIntent(attractionRadiusI, strength);
  
  // Energy → forceScale (linear)
  float forceScaleI = linearMap(intent.getEnergy(), *forceScaleControllerPtr);
  forceScaleControllerPtr->updateIntent(forceScaleI, strength);
  
  // Density → connectionRadius (linear)
  float connectionRadiusI = linearMap(intent.getDensity(), *connectionRadiusControllerPtr);
  connectionRadiusControllerPtr->updateIntent(connectionRadiusI, strength);
  
  // Energy → colourMultiplier (linear)
  float colourMultiplierI = linearMap(intent.getEnergy(), *colourMultiplierControllerPtr);
  colourMultiplierControllerPtr->updateIntent(colourMultiplierI, strength);
  
  // Chaos → maxSpeed (linear: more chaos = faster particles)
  float maxSpeedI = linearMap(intent.getChaos(), *maxSpeedControllerPtr);
  maxSpeedControllerPtr->updateIntent(maxSpeedI, strength);
}


} // ofxMarkSynth
