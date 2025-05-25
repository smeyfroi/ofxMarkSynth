#include "ofApp.h"

//--------------------------------------------------------------
ofxMarkSynth::ModPtrs ofApp::createMods() {
  auto mods = ofxMarkSynth::ModPtrs {};

  auto randomVecSourceModPtr = addMod<ofxMarkSynth::RandomVecSourceMod>(mods, "Random Points", {
    {"CreatedPerUpdate", "0.4"}
  }, 2);
  
  auto particleSetModPtr = addMod<ofxMarkSynth::ParticleSetMod>(mods, "Particles", {});
  randomVecSourceModPtr->addSink(ofxMarkSynth::RandomVecSourceMod::SOURCE_VEC2,
                                 particleSetModPtr,
                                 ofxMarkSynth::ParticleSetMod::SINK_POINTS);
  
  particleSetModPtr->receive(ofxMarkSynth::DrawPointsMod::SINK_FBO, fboPtr);

  return mods;
}

ofxMarkSynth::FboConfigPtrs ofApp::createFboConfigs() {
  ofxMarkSynth::FboConfigPtrs fbos;
  auto fboConfigPtrBackground = std::make_shared<ofxMarkSynth::FboConfig>(fboPtr, nullptr);
  fbos.emplace_back(fboConfigPtrBackground);
  return fbos;
}

void ofApp::setup() {
  ofSetBackgroundColor(0);
  ofDisableArbTex();
  
  fboPtr->allocate(ofGetWindowWidth(), ofGetWindowHeight(), GL_RGBA32F);
  fboPtr->getSource().clearColorBuffer(ofFloatColor(0.0, 0.0, 0.0, 0.0));
  synth.configure(createMods(), createFboConfigs(), ofGetWindowSize());
  
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
  if (synth.keyPressed(key)) return;
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
