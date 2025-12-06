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
  resources.add("fontPath", FONT_PATH);

  synthPtr = ofxMarkSynth::Synth::create("Text", ofxMarkSynth::ModConfig {
  }, START_PAUSED, SYNTH_COMPOSITE_SIZE, resources);

  synthPtr->loadFromConfig(ofToDataPath("1.json"));
  synthPtr->configureGui(nullptr); // nullptr == no imgui window

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
  synthPtr->shutdown();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
  if (key == OF_KEY_TAB) { guiVisible = not guiVisible; return; }
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
