//
//  VideoFlowSourceMod.cpp
//  example_particles_from_video
//
//  Created by Steve Meyfroidt on 09/05/2025.
//

#include "VideoFlowSourceMod.hpp"


namespace ofxMarkSynth {


VideoFlowSourceMod::VideoFlowSourceMod(const std::string& name, const ModConfig&& config, int deviceID, glm::vec2 size, bool saveRecording_, std::string recordingDir_)
: Mod { name, std::move(config) },
saveRecording { saveRecording_ },
recordingDir { recordingDir_ }
{
  motionFromVideo.initialiseCamera(deviceID, size);
}

VideoFlowSourceMod::VideoFlowSourceMod(const std::string& name, const ModConfig&& config, std::string videoFilePath, bool mute)
: Mod { name, std::move(config) }
{
  motionFromVideo.load(videoFilePath, mute);
}

VideoFlowSourceMod::~VideoFlowSourceMod() {
  if (saveRecording) recorder.stop();
}

void VideoFlowSourceMod::initRecorder() {
  recorder.setup(/*video*/true, /*audio*/false, motionFromVideo.getSize(), /*fps*/30.0, /*bitrate*/6000);
  recorder.setOverWrite(true);
  if (recordingDir == "") recordingDir = ofToDataPath("video-flow-recordings");
  std::filesystem::create_directory(recordingDir);
  recorder.setFFmpegPathToAddonsPath();
  recorder.setInputPixelFormat(OF_IMAGE_COLOR);
  recorder.setOutputPath(recordingDir+"/video-flow-recording-"+ofGetTimestampString()+".mp4");
  recorder.startCustomRecord();
}

void VideoFlowSourceMod::initParameters() {
  parameters.add(motionFromVideo.getParameterGroup());
  parameters.add(samplesPerUpdateParameter);
  parameters.add(velocityScaleParameter);
}

void VideoFlowSourceMod::update() {
  motionFromVideo.update();
  
  emit(SOURCE_FLOW_PIXELS, motionFromVideo.getMotionPixels());

  int samples = maxSamplesPerUpdate * samplesPerUpdateParameter;
  glm::vec2 videoSize = motionFromVideo.getSize();
  for (int i = 0; i < samples; i++) {
    if (auto vec = motionFromVideo.trySampleMotion()) {
      emit(SOURCE_VEC4, glm::vec4 {
        vec->x / videoSize.x,
        vec->y / videoSize.y,
        vec->z * velocityScaleParameter / videoSize.x,
        vec->w * velocityScaleParameter / videoSize.y
      });
    }
  }
}

void VideoFlowSourceMod::draw() {
  motionFromVideo.draw();
  
  if (saveRecording) {
    if (!recorder.isRecording()) initRecorder();
    ofPixels pixels;
    motionFromVideo.getVideoFbo().readToPixels(pixels);
    recorder.addFrame(pixels);
  }
}

bool VideoFlowSourceMod::keyPressed(int key) {
  return motionFromVideo.keyPressed(key);
}


} // ofxMarkSynth
