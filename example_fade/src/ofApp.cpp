#include "ofApp.h"
#include "ofxTimeMeasurements.h"

void ofApp::setup() {
  ofDisableArbTex();
  glEnable(GL_PROGRAM_POINT_SIZE);
  ofSetBackgroundColor(0);
  ofSetFrameRate(30);
  TIME_SAMPLE_SET_FRAMERATE(30);

  const glm::vec2 SYNTH_COMPOSITE_SIZE = { 1080, 1080 }; // drawing layers are scaled down to this size to fit into the window height
  const bool START_PAUSED = false;
  
  synthPtr = ofxMarkSynth::Synth::create("Fade", ofxMarkSynth::ModConfig {
  }, START_PAUSED, SYNTH_COMPOSITE_SIZE);
  
  synthPtr->loadFromConfig(ofToDataPath("example_fade.json"));
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
