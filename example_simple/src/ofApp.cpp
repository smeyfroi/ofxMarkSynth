#include "ofApp.h"
#include "ofxTimeMeasurements.h"
#include <stdexcept>

void ofApp::setup() {
  ofSetBackgroundColor(0);
  ofSetFrameRate(FRAME_RATE);
  TIME_SAMPLE_SET_FRAMERATE(FRAME_RATE);

  ofxMarkSynth::ResourceManager resources;
  resources.add("sourceAudioPath", SOURCE_AUDIO_PATH);
  resources.add("audioOutDeviceName", AUDIO_OUT_DEVICE_NAME);
  resources.add("audioBufferSize", AUDIO_BUFFER_SIZE);
  resources.add("audioChannels", AUDIO_CHANNELS);
  resources.add("audioSampleRate", AUDIO_SAMPLE_RATE);

  synthPtr = ofxMarkSynth::Synth::create("Simple", ofxMarkSynth::ModConfig {
  }, START_PAUSED, ofGetWindowSize(), resources);
  if (!synthPtr) {
    ofLogError("example_simple") << "Failed to create Synth";
    throw std::runtime_error("Failed to create Synth");
  }

  synthPtr->loadFromConfig(ofToDataPath("example_simple.json"));
  synthPtr->configureGui(nullptr); // nullptr == no imgui window

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
  if (synthPtr) {
    synthPtr->shutdown();
  }
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
