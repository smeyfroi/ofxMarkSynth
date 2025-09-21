#include "ofApp.h"

using namespace ofxMarkSynth;

//--------------------------------------------------------------
ofxMarkSynth::ModPtrs ofApp::createMods() {
  auto mods = ofxMarkSynth::ModPtrs {};

  auto randomVecSourceModPtr = addMod<ofxMarkSynth::RandomVecSourceMod>(mods, "Random Points", {
    {"CreatedPerUpdate", "0.5"}
  }, 2);
  
  auto particleSetModPtr = addMod<ofxMarkSynth::ParticleSetMod>(mods, "Particles", {});
  randomVecSourceModPtr->addSink(ofxMarkSynth::RandomVecSourceMod::SOURCE_VEC2,
                                 particleSetModPtr,
                                 ofxMarkSynth::ParticleSetMod::SINK_POINTS);
  
  auto snapshotModPtr = addMod<ofxMarkSynth::PixelSnapshotMod>(mods, "Snapshot", {});
  particleSetModPtr->addSink(ofxMarkSynth::ParticleSetMod::SOURCE_FBO,
                             snapshotModPtr,
                             ofxMarkSynth::PixelSnapshotMod::SINK_FBO);
  
  auto pathModPtr = addMod<ofxMarkSynth::PathMod>(mods, "Path", {});
  randomVecSourceModPtr->addSink(ofxMarkSynth::RandomVecSourceMod::SOURCE_VEC2,
                                 pathModPtr,
                                 ofxMarkSynth::PathMod::SINK_VEC2);

  auto randomColourSourceModPtr = addMod<ofxMarkSynth::RandomVecSourceMod>(mods, "Random Colours", {
    {"CreatedPerUpdate", "0.01"}
  }, 4);
  
  auto collageModPtr = addMod<ofxMarkSynth::CollageMod>(mods, "Collage", {});
  snapshotModPtr->addSink(ofxMarkSynth::PixelSnapshotMod::SOURCE_SNAPSHOT,
                          collageModPtr,
                          ofxMarkSynth::CollageMod::SINK_SNAPSHOT_FBO);
  pathModPtr->addSink(ofxMarkSynth::PathMod::SOURCE_PATH,
                      collageModPtr,
                      ofxMarkSynth::CollageMod::SINK_PATH);
  randomColourSourceModPtr->addSink(ofxMarkSynth::RandomVecSourceMod::SOURCE_VEC4,
                      collageModPtr,
                      ofxMarkSynth::CollageMod::SINK_COLOR);

  particleSetModPtr->receive(ofxMarkSynth::ParticleSetMod::SINK_FBO, fboPtr);
  collageModPtr->receive(ofxMarkSynth::CollageMod::SINK_FBO, fboPtr);

  return mods;
}

ofxMarkSynth::FboConfigPtrs ofApp::createFboConfigs() {
  ofxMarkSynth::FboConfigPtrs fboConfigPtrs;
  addFboConfigPtr(fboConfigPtrs, "foreground", fboPtr, {ofGetWindowWidth(), ofGetWindowHeight()}, GL_RGBA8, GL_CLAMP_TO_EDGE, ofColor::black, false, OF_BLENDMODE_ALPHA, true, 0); // useStencil for CollageMod
  return fboConfigPtrs;
}

void ofApp::setup() {
  ofSetBackgroundColor(0);
  ofDisableArbTex();
  ofSetFrameRate(60);
  
  synth = std::make_shared<Synth>("Synth", ModConfig {});
  synth->configure(createFboConfigs(), createMods(), ofGetWindowSize());
  
  parameters.add(synth->getParameterGroup());
  gui.setup(parameters);
}

//--------------------------------------------------------------
void ofApp::update(){
  synth->update();
}

//--------------------------------------------------------------
void ofApp::draw(){
  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  synth->draw();
  if (guiVisible) gui.draw();
}

//--------------------------------------------------------------
void ofApp::exit(){
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
  if (key == OF_KEY_TAB) guiVisible = not guiVisible;
  if (synth->keyPressed(key)) return;
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
