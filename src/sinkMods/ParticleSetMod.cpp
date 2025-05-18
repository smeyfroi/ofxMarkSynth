//
//  ParticleSetMod.cpp
//  example_particles
//
//  Created by Steve Meyfroidt on 09/05/2025.
//

#include "ParticleSetMod.hpp"


namespace ofxMarkSynth {


ParticleSetMod::ParticleSetMod(const std::string& name, const ModConfig&& config)
: Mod { name, std::move(config) }
{}

void ParticleSetMod::initParameters() {
  parameters.add(spinParameter);
  parameters.add(colorParameter);
  parameters.add(particleSet.getParameterGroup());
}

void ParticleSetMod::update() {
  auto fboPtr = fboPtrs[0];
  if (fboPtr == nullptr) return;
  particleSet.forceScale = 1.0 / fboPtr->getWidth();
  std::for_each(newPoints.begin(),
                newPoints.end(),
                [this](const auto& vec) {
    glm::vec2 p { vec.x, vec.y };
    glm::vec2 v { vec.z, vec.w };
    particleSet.add(p, v, colorParameter, spinParameter);
  });
  newPoints.clear();
  
  fboPtr->getSource().begin();
  ofScale(fboPtr->getWidth(), fboPtr->getHeight());
  particleSet.draw();
  fboPtr->getSource().end();
}

void ParticleSetMod::receive(int sinkId, const float& value) {
  switch (sinkId) {
    case SINK_SPIN:
      spinParameter = value;
      break;
    default:
      ofLogError() << "float receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void ParticleSetMod::receive(int sinkId, const glm::vec2& point) {
  switch (sinkId) {
    case SINK_POINTS:
      newPoints.push_back(glm::vec4 { point, ofRandom(0.01)-0.005, ofRandom(0.01)-0.005 });
      break;
    default:
      ofLogError() << "glm::vec2 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void ParticleSetMod::receive(int sinkId, const glm::vec4& v) {
  switch (sinkId) {
    case SINK_POINT_VELOCITIES:
      newPoints.push_back(v);
      break;
    case SINK_COLOR:
      colorParameter = ofFloatColor { v.r, v.g, v.b, v.a };
      break;
    default:
      ofLogError() << "glm::vec4 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}


} // ofxMarkSynth
