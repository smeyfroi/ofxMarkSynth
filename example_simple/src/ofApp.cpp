#include "ofApp.h"
#include <memory>

void ofApp::setup() {
  ofSetBackgroundColor(0);
  ofSetFrameRate(30);
    
  synthPtr = std::make_shared<ofxMarkSynth::Synth>("Simple", ofxMarkSynth::ModConfig {
    {"Back Color", "0.0, 0.0, 0.0, 1.0"},
  }, false, ofGetWindowSize());

  auto randomVecSourceModPtr = synthPtr->addMod<ofxMarkSynth::RandomVecSourceMod>("Random Points", {
    {"CreatedPerUpdate", "0.4"}
  }, 2);
  
  auto pointIntrospectorModPtr = synthPtr->addMod<ofxMarkSynth::IntrospectorMod>("Introspector", {});

  ofxMarkSynth::connectSourceToSinks(randomVecSourceModPtr, {
    { ofxMarkSynth::RandomVecSourceMod::SOURCE_VEC2,
      {{ pointIntrospectorModPtr, ofxMarkSynth::IntrospectorMod::SINK_POINTS }}
    }
  });
  
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
