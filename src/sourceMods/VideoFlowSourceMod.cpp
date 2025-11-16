//
//  VideoFlowSourceMod.cpp
//  example_particles_from_video
//
//  Created by Steve Meyfroidt on 09/05/2025.
//

#include "VideoFlowSourceMod.hpp"



namespace ofxMarkSynth {



VideoFlowSourceMod::VideoFlowSourceMod(Synth* synthPtr, const std::string& name, ModConfig config, const std::filesystem::path& sourceVideoFilePath, bool mute)
: Mod { synthPtr, name, std::move(config) }
{
  motionFromVideo.load(sourceVideoFilePath, mute);
  
  sourceNameIdMap = {
    { "flowFbo", SOURCE_FLOW_FBO }
  };
}

VideoFlowSourceMod::VideoFlowSourceMod(Synth* synthPtr, const std::string& name, ModConfig config, int deviceID, glm::vec2 size, bool saveRecording_, const std::filesystem::path& recordingDir_)
: Mod { synthPtr, name, std::move(config) },
saveRecording { saveRecording_ },
recordingDir { recordingDir_ }
{
  motionFromVideo.initialiseCamera(deviceID, size);
  
  sourceNameIdMap = {
    { "flowFbo", SOURCE_FLOW_FBO }
  };
}

VideoFlowSourceMod::~VideoFlowSourceMod() {
#ifdef TARGET_MAC
  if (saveRecording) recorder.stop();
#endif
}

#ifdef TARGET_MAC
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
#endif

void VideoFlowSourceMod::initParameters() {
  parameters.add(motionFromVideo.getParameterGroup());
}

void VideoFlowSourceMod::update() {
  motionFromVideo.update();
  
  if (motionFromVideo.isReady()) {
    emit(SOURCE_FLOW_FBO, motionFromVideo.getMotionFbo());
  }
}

void VideoFlowSourceMod::draw() {
  motionFromVideo.draw();
  
#ifdef TARGET_MAC
  if (saveRecording) {
    if (!recorder.isRecording()) initRecorder();
    ofPixels pixels;
    motionFromVideo.getVideoFbo().readToPixels(pixels);
    recorder.addFrame(pixels);
  }
#endif
}

bool VideoFlowSourceMod::keyPressed(int key) {
  return motionFromVideo.keyPressed(key);
}



} // ofxMarkSynth
