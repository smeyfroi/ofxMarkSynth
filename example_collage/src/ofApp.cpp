#include "ofApp.h"
#include <stdexcept>
#include "ofxTimeMeasurements.h"

using namespace ofxMarkSynth;

//--------------------------------------------------------------
void ofApp::setup() {
  ofDisableArbTex();
  glEnable(GL_PROGRAM_POINT_SIZE);
  ofSetBackgroundColor(0);
  ofSetFrameRate(FRAME_RATE);
  TIME_SAMPLE_SET_FRAMERATE(FRAME_RATE);
  
  ResourceManager resources;
  resources.add("performanceConfigRootPath", PERFORMANCE_CONFIG_ROOT_PATH);
  resources.add("performanceArtefactRootPath", PERFORMANCE_ARTEFACT_ROOT_PATH);
  //  resources.add("compositeSize", COMPOSITE_SIZE);
  resources.add("compositePanelGapPx", COMPOSITE_PANEL_GAP_PX);
  resources.add("recorderCompositeSize", VIDEO_RECORDER_SIZE);
  resources.add("ffmpegBinaryPath", FFMPEG_BINARY_PATH);

  // Audio resources (Synth-owned)
  resources.add("sourceAudioPath", SOURCE_AUDIO_PATH);
  resources.add("audioOutDeviceName", AUDIO_OUT_DEVICE_NAME);
  resources.add("audioBufferSize", AUDIO_BUFFER_SIZE);
  resources.add("audioChannels", AUDIO_CHANNELS);
  resources.add("audioSampleRate", AUDIO_SAMPLE_RATE);

  synthPtr = ofxMarkSynth::Synth::create("example_collage", ofxMarkSynth::ModConfig {
  }, START_PAUSED, COMPOSITE_SIZE, resources);
  if (!synthPtr) {
    ofLogError("example_collage") << "Failed to create Synth";
    throw std::runtime_error("Failed to create Synth");
  }


  synthPtr->loadFromConfig(ofToDataPath("1.json"));
  synthPtr->configureGui(guiWindowPtr);
}

//--------------------------------------------------------------
void ofApp::update(){
  synthPtr->update();
}

//--------------------------------------------------------------
void ofApp::draw(){
  synthPtr->draw();
}

// >>> imgui
void ofApp::drawGui(ofEventArgs& args){
  synthPtr->drawGui();
}
// <<< imgui

//--------------------------------------------------------------
void ofApp::exit(){
  synthPtr->shutdown();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
  if (synthPtr->keyPressed(key)) return;
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
  
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){
  
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
  
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
  
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
  
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
  
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){
  
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){
  
}
