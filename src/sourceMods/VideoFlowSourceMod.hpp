//
//  VideoFlowSourceMod.hpp
//  example_particles_from_video
//
//  Created by Steve Meyfroidt on 09/05/2025.
//

#pragma once

#include "Mod.hpp"
#include "ofxMotionFromVideo.h"


namespace ofxMarkSynth {


class VideoFlowSourceMod : public Mod {

public:
  VideoFlowSourceMod(const std::string& name, const ModConfig&& config, std::string videoFilePath, bool mute);
  void update() override;
  
  static constexpr int SOURCE_VEC4 = 4; // { x, y, dx, dy }

protected:
  void initParameters() override;

private:
  MotionFromVideo motionFromVideo;
  
  int maxSamplesPerUpdate = 1000;
  ofParameter<float> samplesPerUpdateParameter { "SamplesPerUpdate", 0.1, 0.0, 1.0 };
  ofParameter<float> velocityScaleParameter {"velocityScale", 1.0, 0.0, 10.0};

};


} // ofxMarkSynth
