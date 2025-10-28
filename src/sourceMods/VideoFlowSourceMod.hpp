//
//  VideoFlowSourceMod.hpp
//  example_particles_from_video
//
//  Created by Steve Meyfroidt on 09/05/2025.
//

#pragma once

#include "Mod.hpp"
#include "ofxMotionFromVideo.h"
#ifdef TARGET_MAC
#include "ofxFFmpegRecorder.h"
#endif


namespace ofxMarkSynth {


class VideoFlowSourceMod : public Mod {

public:
  VideoFlowSourceMod(Synth* synthPtr, const std::string& name, const ModConfig&& config, int deviceID, glm::vec2 size, bool saveRecording, std::string recordingPath);
  VideoFlowSourceMod(Synth* synthPtr, const std::string& name, const ModConfig&& config, std::string videoFilePath, bool mute);
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
#ifdef TARGET_MAC
  ofxFFmpegRecorder recorder;
  void initRecorder();
#endif
};


} // ofxMarkSynth
