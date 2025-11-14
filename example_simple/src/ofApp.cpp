#include "ofApp.h"
#include <memory>
#include "ofxTimeMeasurements.h"

void ofApp::setup() {
  ofSetBackgroundColor(0);
  ofSetFrameRate(30);
  TIME_SAMPLE_SET_FRAMERATE(30);

  synthPtr = std::make_shared<ofxMarkSynth::Synth>("Simple", ofxMarkSynth::ModConfig {
    {"Back Color", "0.0, 0.0, 0.0, 1.0"},
  }, false, ofGetWindowSize());
  
  synthPtr->loadFromConfig(ofToDataPath("example_simple.json"));
  synthPtr->configureGui(nullptr); // nullptr == no imgui

// >>> Manual equivlent of example_simple.json <<<
//  auto randomVecSourceModPtr = synthPtr->addMod<ofxMarkSynth::RandomVecSourceMod>("Random Points", {
//    {"CreatedPerUpdate", "0.4"}
//  }, 2);
//
//  auto pointIntrospectorModPtr = synthPtr->addMod<ofxMarkSynth::IntrospectorMod>("Introspector", {});
//
//  ofxMarkSynth::connectSourceToSinks(randomVecSourceModPtr, {
//    { ofxMarkSynth::RandomVecSourceMod::SOURCE_VEC2,
//      {{ pointIntrospectorModPtr, ofxMarkSynth::IntrospectorMod::SINK_POINTS }}
//    }
//  });
}

//--------------------------------------------------------------
void ofApp::update(){
  synthPtr->update();
}

//--------------------------------------------------------------
void ofApp::draw(){
  synthPtr->draw();
}

//--------------------------------------------------------------
void ofApp::exit(){
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
