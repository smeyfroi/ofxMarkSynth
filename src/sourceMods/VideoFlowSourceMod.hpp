//
//  VideoFlowSourceMod.hpp
//  example_particles_from_video
//
//  Created by Steve Meyfroidt on 09/05/2025.
//

#pragma once

#include "Mod.hpp"
#include "ofxMotionFromVideo.h"
#ifndef TARGET_OS_IOS
#include "ofxFFmpegRecorder.h"
#endif


namespace ofxMarkSynth {


class VideoFlowSourceMod : public Mod {

public:
  VideoFlowSourceMod(const std::string& name, const ModConfig&& config, int deviceID, glm::vec2 size, bool saveRecording, std::string recordingPath);
  VideoFlowSourceMod(const std::string& name, const ModConfig&& config, std::string videoFilePath, bool mute);
  ~VideoFlowSourceMod();
  void update() override;
  void draw() override;
  bool keyPressed(int key) override;

  static constexpr int SOURCE_FLOW_FBO = 10;

protected:
  void initParameters() override;

private:
  MotionFromVideo motionFromVideo;
  
  bool saveRecording;
  std::string recordingDir;
#ifndef TARGET_OS_IOS
  ofxFFmpegRecorder recorder;
  void initRecorder();
#endif
};


} // ofxMarkSynth
