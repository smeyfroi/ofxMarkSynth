#include "ofApp.h"
#include "ofxTimeMeasurements.h"
#include "ModFactory.hpp"

void ofApp::setup() {
  ofDisableArbTex();
  glEnable(GL_PROGRAM_POINT_SIZE);
  ofSetBackgroundColor(0);
  ofSetFrameRate(FRAME_RATE);
  TIME_SAMPLE_SET_FRAMERATE(FRAME_RATE);
  
  ofxMarkSynth::ResourceManager resources;
  resources.add("sourceVideoPath", SOURCE_VIDEO_PATH);
  resources.add("sourceVideoMute", SOURCE_VIDEO_MUTE);
  resources.add("sourceAudioPath", SOURCE_AUDIO_PATH);
  resources.add("micDeviceName", MIC_DEVICE_NAME);
  resources.add("recordAudio", RECORD_AUDIO);
  resources.add("recordingPath", RECORDING_PATH);

  synthPtr = std::make_shared<ofxMarkSynth::Synth>("Fade", ofxMarkSynth::ModConfig {
  }, START_PAUSED, SYNTH_COMPOSITE_SIZE, resources);

  synthPtr->loadFromConfig(ofToDataPath("2.json"));
  synthPtr->configureGui(nullptr); // nullptr == no imgui window

  ofAddListener(synthPtr->hibernationCompleteEvent, this, &ofApp::onSynthHibernationComplete);

  // No imgui; we manage an ofxGui here instead
  parameters.add(synthPtr->getParameterGroup());
  gui.setup(parameters);
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
  ofRemoveListener(synthPtr->hibernationCompleteEvent, this, &ofApp::onSynthHibernationComplete);
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

//--------------------------------------------------------------
void ofApp::onSynthHibernationComplete(ofxMarkSynth::Synth::HibernationCompleteEvent& e){
  ofLogNotice("ofApp") << "Hibernation complete! Duration: " << e.fadeDuration 
                       << "s, Synth: " << e.synthName;
}
