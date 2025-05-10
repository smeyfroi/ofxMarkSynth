//
//  ParticleSetMod.cpp
//  example_particles
//
//  Created by Steve Meyfroidt on 09/05/2025.
//

#include "ParticleSetMod.hpp"


namespace ofxMarkSynth {


ParticleSetMod::ParticleSetMod(const std::string& name, const ModConfig&& config, const glm::vec2 fboSize)
: Mod { name, std::move(config) },
particleSet { fboSize.x }
{
  fbo.allocate(fboSize.x, fboSize.y, GL_RGBA32F); // 32F to accommodate fade, but this could be an optional thing to use a smaller FBO if no fade
  fbo.getSource().clearColorBuffer(ofFloatColor(0.0, 0.0, 0.0, 0.0));
  fadeShader.load();
}

void ParticleSetMod::initParameters() {
  parameters.add(spinParameter);
  parameters.add(colorParameter);
  parameters.add(fadeParameter);
  parameters.add(particleSet.getParameterGroup());
}

void ParticleSetMod::update() {
  std::for_each(newPoints.begin(),
                newPoints.end(),
                [this](const auto& vec) {
    glm::vec2 p { vec.x, vec. y };
    glm::vec2 v { vec.z, vec. w };
    particleSet.add(p * fbo.getWidth(), v, colorParameter, spinParameter);
  });
  newPoints.clear();
  
  glm::vec4 fade { fadeParameter->r, fadeParameter->g, fadeParameter->b, fadeParameter->a };
  fadeShader.render(fbo, fade);

  fbo.getSource().begin();
//  ofScale(fbo.getWidth(), fbo.getHeight());
  particleSet.draw();
  fbo.getSource().end();
}

void ParticleSetMod::draw() {
  fbo.draw(0.0, 0.0);
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
      newPoints.push_back(glm::vec4 { point, ofRandom(0.1)-0.05, ofRandom(0.1)-0.05 });
      break;
    default:
      ofLogError() << "glm::vec2 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void ParticleSetMod::receive(int sinkId, const glm::vec4& v) {
  switch (sinkId) {
    case SINK_POINT_VELOCITIES:
      newPoints.push_back(v);
    case SINK_COLOR:
      colorParameter = ofFloatColor { v.r, v.g, v.b, v.a };
      break;
    default:
      ofLogError() << "glm::vec4 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}


} // ofxMarkSynth
