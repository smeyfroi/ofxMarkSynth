#include "ofApp.h"

//--------------------------------------------------------------
std::unique_ptr<ofxMarkSynth::ModPtrs> ofApp::createMods() {
  auto mods = std::make_unique<ofxMarkSynth::ModPtrs>();

  auto videoFlowSourceModPtr = std::make_shared<ofxMarkSynth::VideoFlowSourceMod>("Video", ofxMarkSynth::ModConfig {
  }, ofToDataPath("trimmed.mov"), true);
  mods->push_back(videoFlowSourceModPtr);
  
  auto particleSetModPtr = std::make_shared<ofxMarkSynth::ParticleSetMod>("Particles", ofxMarkSynth::ModConfig {
  }, ofGetWindowSize());
  videoFlowSourceModPtr->addSink(ofxMarkSynth::VideoFlowSourceMod::SOURCE_VEC4,
                                 particleSetModPtr,
                                 ofxMarkSynth::ParticleSetMod::SINK_POINT_VELOCITIES);
  mods->push_back(particleSetModPtr);
  
  return mods;
}

void ofApp::setup() {
  ofSetBackgroundColor(0);
  ofDisableArbTex();
  
  synth.configure(createMods());
  
  parameters.add(synth.getParameterGroup("Synth"));
  gui.setup(parameters);
}

//--------------------------------------------------------------
void ofApp::update(){
  synth.update();
}

//--------------------------------------------------------------
void ofApp::draw(){
  synth.draw();

  if (guiVisible) gui.draw();
}

//--------------------------------------------------------------
void ofApp::exit(){
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
  if (key == OF_KEY_TAB) guiVisible = not guiVisible;
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
