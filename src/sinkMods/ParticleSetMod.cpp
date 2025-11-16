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

void ParticleSetMod::initParameters() {
  parameters.add(spinParameter);
  parameters.add(colorParameter);
  addFlattenedParameterGroup(parameters, particleSet.getParameterGroup());
}

void ParticleSetMod::update() {
  spinController.update();
  colorController.update();
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
  ofEnableBlendMode(OF_BLENDMODE_DISABLED);
  ofScale(fboPtr->getWidth(), fboPtr->getHeight());
  particleSet.draw();
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
  spinController.updateIntent(ofxMarkSynth::linearMap(intent.getEnergy() * intent.getChaos(), -0.05f, 0.05f), strength);
  ofFloatColor color = ofxMarkSynth::energyToColor(intent);
  color.setBrightness(ofxMarkSynth::structureToBrightness(intent));
  color.setSaturation(intent.getEnergy() * intent.getChaos() * (1.0f - intent.getStructure()));
  color.a = ofxMarkSynth::linearMap(intent.getDensity(), 0.0f, 1.0f);
  colorController.updateIntent(color, strength);
}


} // ofxMarkSynth
