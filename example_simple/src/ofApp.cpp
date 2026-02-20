#include "ofApp.h"
#include "ofxTimeMeasurements.h"
#include <stdexcept>

void ofApp::setup() {
  ofDisableArbTex();
  glEnable(GL_PROGRAM_POINT_SIZE);
  ofSetBackgroundColor(0);
  ofSetFrameRate(FRAME_RATE);
  TIME_SAMPLE_SET_FRAMERATE(FRAME_RATE);

  ofxMarkSynth::ResourceManager resources;
  resources.add("sourceAudioPath", SOURCE_AUDIO_PATH);
  resources.add("audioOutDeviceName", AUDIO_OUT_DEVICE_NAME);
  resources.add("audioBufferSize", AUDIO_BUFFER_SIZE);
  resources.add("audioChannels", AUDIO_CHANNELS);
  resources.add("audioSampleRate", AUDIO_SAMPLE_RATE);
  resources.add("compositePanelGapPx", COMPOSITE_PANEL_GAP_PX);
  resources.add("recorderCompositeSize", VIDEO_RECORDER_SIZE);
  resources.add("ffmpegBinaryPath", FFMPEG_BINARY_PATH);

  synthPtr = ofxMarkSynth::Synth::create("Simple", ofxMarkSynth::ModConfig {
  }, START_HIBERNATED, COMPOSITE_SIZE, resources);
  if (!synthPtr) {
    ofLogError("example_simple") << "Failed to create Synth";
    throw std::runtime_error("Failed to create Synth");
  }

  synthPtr->loadFromConfig(ofToDataPath("example_simple.json"));
  synthPtr->configureGui(nullptr); // nullptr == no imgui window
}

void ofApp::update(){
  synthPtr->update();
}

void ofApp::draw(){
  synthPtr->draw();
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
