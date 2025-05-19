#include "ofApp.h"

//--------------------------------------------------------------
ofxMarkSynth::ModPtrs ofApp::createMods() {
  auto mods = ofxMarkSynth::ModPtrs {};

  auto randomVecSourceModPtr = std::make_shared<ofxMarkSynth::RandomVecSourceMod>("Random Points", ofxMarkSynth::ModConfig {
    {"CreatedPerUpdate", "0.4"}
  }, 2);
  mods.push_back(randomVecSourceModPtr);
  
  ofxMarkSynth::ModPtr particleSetModPtr = std::make_shared<ofxMarkSynth::ParticleSetMod>("Particles", ofxMarkSynth::ModConfig {
  });
  randomVecSourceModPtr->addSink(ofxMarkSynth::RandomVecSourceMod::SOURCE_VEC2,
                                 particleSetModPtr,
                                 ofxMarkSynth::ParticleSetMod::SINK_POINTS);
  mods.push_back(particleSetModPtr);
  
  ofxMarkSynth::ModPtr pixelSnapshotModPtr = std::make_shared<ofxMarkSynth::PixelSnapshotMod>("Snapshot", ofxMarkSynth::ModConfig {
  });
  particleSetModPtr->addSink(ofxMarkSynth::ParticleSetMod::SOURCE_FBO,
                             pixelSnapshotModPtr,
                             ofxMarkSynth::PixelSnapshotMod::SINK_FBO);
  mods.push_back(pixelSnapshotModPtr);
  
  ofxMarkSynth::ModPtr pathModPtr = std::make_shared<ofxMarkSynth::PathMod>("Path", ofxMarkSynth::ModConfig {
  });
  randomVecSourceModPtr->addSink(ofxMarkSynth::RandomVecSourceMod::SOURCE_VEC2,
                                 pathModPtr,
                                 ofxMarkSynth::PathMod::SINK_VEC2);
  mods.push_back(pathModPtr);

  // mask from path
  
  // collage into mask from snapshot
  
  particleSetModPtr->receive(ofxMarkSynth::ParticleSetMod::SINK_FBO, fboPtr);

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
