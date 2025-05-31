//
//  VideoFlowSourceMod.hpp
//  example_particles_from_video
//
//  Created by Steve Meyfroidt on 09/05/2025.
//

#pragma once

#include "Mod.hpp"
#include "ofxMotionFromVideo.h"
#include "ofxFFmpegRecorder.h"


namespace ofxMarkSynth {


class VideoFlowSourceMod : public Mod {

public:
  VideoFlowSourceMod(const std::string& name, const ModConfig&& config, int deviceID, glm::vec2 size, bool saveRecording, std::string recordingPath);
  VideoFlowSourceMod(const std::string& name, const ModConfig&& config, std::string videoFilePath, bool mute);
  ~VideoFlowSourceMod();
  void update() override;
  void draw() override;
  bool keyPressed(int key) override;

  static constexpr int SOURCE_FLOW_PIXELS = 1;
  static constexpr int SOURCE_VEC4 = 4; // { x, y, dx, dy }

protected:
  void initParameters() override;

private:
  MotionFromVideo motionFromVideo;
  
  int maxSamplesPerUpdate = 1000;
  ofParameter<float> samplesPerUpdateParameter { "SamplesPerUpdate", 0.1, 0.0, 1.0 };
  ofParameter<float> velocityScaleParameter {"velocityScale", 1.0, 0.0, 10.0};
  
  bool saveRecording;
  std::string recordingDir;
  ofxFFmpegRecorder recorder;
  void initRecorder();

};


} // ofxMarkSynth
