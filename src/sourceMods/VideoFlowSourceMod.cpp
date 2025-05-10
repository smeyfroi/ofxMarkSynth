//
//  VideoFlowSourceMod.cpp
//  example_particles_from_video
//
//  Created by Steve Meyfroidt on 09/05/2025.
//

#include "VideoFlowSourceMod.hpp"


namespace ofxMarkSynth {


VideoFlowSourceMod::VideoFlowSourceMod(const std::string& name, const ModConfig&& config, std::string videoFilePath, bool mute)
: Mod { name, std::move(config) }
{
  motionFromVideo.load(videoFilePath, mute);
}

void VideoFlowSourceMod::initParameters() {
  parameters.add(motionFromVideo.getParameterGroup());
  parameters.add(samplesPerUpdateParameter);
  parameters.add(velocityScaleParameter);
}

void VideoFlowSourceMod::update() {
  motionFromVideo.update();

  int samples = maxSamplesPerUpdate * samplesPerUpdateParameter;
  glm::vec2 videoSize = motionFromVideo.getSize();
  for (int i = 0; i < samples; i++) {
    if (auto vec = motionFromVideo.trySampleMotion()) {
      emit(SOURCE_VEC4, glm::vec4 {vec->x / videoSize.x, vec->y / videoSize.y, vec->z * velocityScaleParameter, vec->w * velocityScaleParameter});
    }
  }
}


} // ofxMarkSynth
