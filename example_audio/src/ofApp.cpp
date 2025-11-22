#include "ofApp.h"
#include <memory>
#include "ofxTimeMeasurements.h"

void ofApp::setup() {
  ofSetBackgroundColor(0);
  ofSetFrameRate(30);
  TIME_SAMPLE_SET_FRAMERATE(30);

  ofxMarkSynth::ResourceManager resources;
  resources.add("sourceAudioPath", SOURCE_AUDIO_PATH);
  resources.add("micDeviceName", MIC_DEVICE_NAME);
  resources.add("recordAudio", RECORD_AUDIO);
  resources.add("recordingPath", RECORDING_PATH);

  synthPtr = std::make_shared<ofxMarkSynth::Synth>("Audio", ofxMarkSynth::ModConfig {
  }, START_PAUSED, SYNTH_COMPOSITE_SIZE, resources);

  synthPtr->loadFromConfig(ofToDataPath("example_audio.json"));
  synthPtr->configureGui(nullptr); // nullptr == no imgui window

  parameters.add(synthPtr->getParameterGroup());
  gui.setup(parameters);
  gui.getGroup("Synth").minimizeAll();
}

//--------------------------------------------------------------
void ofApp::update(){
  synthPtr->update();
}

//--------------------------------------------------------------
void ofApp::draw(){
  synthPtr->draw();
  if (guiVisible) gui.draw();
}

//--------------------------------------------------------------
void ofApp::exit(){
  synthPtr->shutdown();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
  if (key == OF_KEY_TAB) guiVisible = not guiVisible;
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
