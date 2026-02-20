#include "ofApp.h"
#include <stdexcept>
#include "ofxTimeMeasurements.h"

void ofApp::setup() {
  ofDisableArbTex();
  glEnable(GL_PROGRAM_POINT_SIZE);
  ofSetBackgroundColor(0);
  ofSetFrameRate(FRAME_RATE);
  TIME_SAMPLE_SET_FRAMERATE(FRAME_RATE);

  ofxMarkSynth::ResourceManager resources;
  resources.add("performanceConfigRootPath", PERFORMANCE_CONFIG_ROOT_PATH);
  resources.add("performanceArtefactRootPath", PERFORMANCE_ARTEFACT_ROOT_PATH);
  resources.add("compositePanelGapPx", COMPOSITE_PANEL_GAP_PX);
  resources.add("recorderCompositeSize", VIDEO_RECORDER_SIZE);
  resources.add("ffmpegBinaryPath", FFMPEG_BINARY_PATH);

  // Audio resources (Synth-owned)
  resources.add("sourceAudioPath", SOURCE_AUDIO_PATH);
  resources.add("audioOutDeviceName", AUDIO_OUT_DEVICE_NAME);
  resources.add("audioBufferSize", AUDIO_BUFFER_SIZE);
  resources.add("audioChannels", AUDIO_CHANNELS);
  resources.add("audioSampleRate", AUDIO_SAMPLE_RATE);
  resources.add("sourceVideoPath", SOURCE_VIDEO_PATH);
  resources.add("sourceVideoMute", SOURCE_VIDEO_MUTE);
  resources.add("cameraDeviceId", CAMERA_DEVICE_ID);
  resources.add("videoSize", VIDEO_SIZE);
  resources.add("saveRecording", SAVE_RECORDING);
  resources.add("videoRecordingPath", VIDEO_RECORDING_PATH);

  synthPtr = ofxMarkSynth::Synth::create("example_particles_from_video", ofxMarkSynth::ModConfig {
  }, START_HIBERNATED, COMPOSITE_SIZE, resources);
  if (!synthPtr) {
    ofLogError("example_particles_from_video") << "Failed to create Synth";
    throw std::runtime_error("Failed to create Synth");
  }

  synthPtr->loadFromConfig(ofToDataPath("1.json"));
  synthPtr->configureGui(guiWindowPtr);
}

void ofApp::update(){
  synthPtr->update();
}

void ofApp::draw(){
  synthPtr->draw();
}

void ofApp::drawGui(ofEventArgs& args){
  synthPtr->drawGui();
}

void ofApp::exit(){
  if (synthPtr) {
    synthPtr->shutdown();
  }
}

void ofApp::keyPressed(int key){
  if (synthPtr->keyPressed(key)) return;
}

void ofApp::keyReleased(int key){
}

void ofApp::windowResized(int w, int h){
  if (synthPtr) synthPtr->windowResized(w, h);
}
