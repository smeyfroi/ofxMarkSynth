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
{
  addTextureThresholdedShader.load();
}

void ParticleSetMod::initParameters() {
  parameters.add(spinParameter);
  parameters.add(colorParameter);
  parameters.add(particleSet.getParameterGroup());
}

void ParticleSetMod::initTempFbo() {
  if (! tempFbo.isAllocated()) {
    auto fboPtr = fboPtrs[0];
    ofFboSettings settings { nullptr };
    settings.wrapModeVertical = GL_REPEAT;
    settings.wrapModeHorizontal = GL_REPEAT;
    settings.width = fboPtr->getWidth();
    settings.height = fboPtr->getHeight();
    settings.internalformat = GL_RGBA32F;
    settings.numSamples = 0;
    settings.useDepth = true;
    settings.useStencil = true;
    settings.textureTarget = ofGetUsingArbTex() ? GL_TEXTURE_RECTANGLE_ARB : GL_TEXTURE_2D;
    tempFbo.allocate(settings);
  }
}

void ParticleSetMod::update() {
  auto fboPtr = fboPtrs[0];
  if (fboPtr == nullptr) return;
  
  particleSet.update();
  
  std::for_each(newPoints.begin(),
                newPoints.end(),
                [this](const auto& vec) {
    glm::vec2 p { vec.x, vec.y };
    glm::vec2 v { vec.z, vec.w };
    particleSet.add(p, v, colorParameter, spinParameter);
  });
  newPoints.clear();
  
  initTempFbo();
  tempFbo.begin();
  ofClear(0, 0, 0, 0);
  ofScale(fboPtr->getWidth(), fboPtr->getHeight());
  particleSet.draw();
  tempFbo.end();

  addTextureThresholdedShader.render(*fboPtr, tempFbo);
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
