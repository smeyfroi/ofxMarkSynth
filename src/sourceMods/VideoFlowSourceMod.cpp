//
//  VideoFlowSourceMod.cpp
//  example_particles_from_video
//
//  Created by Steve Meyfroidt on 09/05/2025.
//

#include "VideoFlowSourceMod.hpp"
#include "IntentMapping.hpp"
#include "Parameter.hpp"



namespace ofxMarkSynth {



VideoFlowSourceMod::VideoFlowSourceMod(Synth* synthPtr, const std::string& name, ModConfig config, const std::filesystem::path& sourceVideoFilePath, bool mute)
: Mod { synthPtr, name, std::move(config) },
saveRecording { false }
{
  motionFromVideo.load(sourceVideoFilePath, mute);
  
  sourceNameIdMap = {
    { "flowFbo", SOURCE_FLOW_FBO },
    { "pointVelocity", SOURCE_POINT_VELOCITY }
  };
}

VideoFlowSourceMod::VideoFlowSourceMod(Synth* synthPtr, const std::string& name, ModConfig config, int deviceID, glm::vec2 size, bool saveRecording_, const std::filesystem::path& recordingDir_)
: Mod { synthPtr, name, std::move(config) },
saveRecording { saveRecording_ },
recordingDir { recordingDir_ }
{
  motionFromVideo.initialiseCamera(deviceID, size);
  
  sourceNameIdMap = {
    { "flowFbo", SOURCE_FLOW_FBO },
    { "pointVelocity", SOURCE_POINT_VELOCITY }
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
  addFlattenedParameterGroup(parameters, motionFromVideo.getParameterGroup());
}

void VideoFlowSourceMod::update() {
  motionFromVideo.update();
  
  if (motionFromVideo.isReady()) {
    emit(SOURCE_FLOW_FBO, motionFromVideo.getMotionFbo());
  }
  
  // TODO: make this a separate process Mod that can sample a texture
  // TODO: make the number of samples a parameter
  if (motionFromVideo.isReady()) {
    for (int i = 0; i < 100; i++) {
      if (auto vec = motionFromVideo.trySampleMotion()) {
        emit(SOURCE_POINT_VELOCITY, vec.value());
      }
    }
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
