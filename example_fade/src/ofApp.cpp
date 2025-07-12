#include "ofApp.h"

//--------------------------------------------------------------
ofxMarkSynth::ModPtrs ofApp::createMods() {
  auto mods = ofxMarkSynth::ModPtrs {};

  auto randomVecSourceModPtr = addMod<ofxMarkSynth::RandomVecSourceMod>(mods, "Random Points", {
    {"CreatedPerUpdate", "0.4"}
  }, 2);
  
  auto drawPointsModPtr = addMod<ofxMarkSynth::DrawPointsMod>(mods, "Draw Points", {});
  randomVecSourceModPtr->addSink(ofxMarkSynth::RandomVecSourceMod::SOURCE_VEC2,
                                 drawPointsModPtr,
                                 ofxMarkSynth::DrawPointsMod::SINK_POINTS);
  
  auto translateModPtr = addMod<ofxMarkSynth::TranslateMod>(mods, "Translate", {});
  drawPointsModPtr->addSink(ofxMarkSynth::DrawPointsMod::SOURCE_FBO,
                            translateModPtr,
                            ofxMarkSynth::TranslateMod::SINK_FBO);
  
  auto multiplyModPtr = addMod<ofxMarkSynth::MultiplyMod>(mods, "Fade", {});
  translateModPtr->addSink(ofxMarkSynth::TranslateMod::SOURCE_FBO,
                           multiplyModPtr,
                           ofxMarkSynth::MultiplyMod::SINK_FBO);

  drawPointsModPtr->receive(ofxMarkSynth::DrawPointsMod::SINK_FBO, fboPtr);

  return mods;
}

ofxMarkSynth::FboConfigPtrs ofApp::createFboConfigs() {
  ofxMarkSynth::FboConfigPtrs fbos;
  addFboConfigPtr(fbos, "foreground", fboPtr, ofGetWindowSize()*8.0, GL_RGBA32F, GL_REPEAT, ofColor::black, false, OF_BLENDMODE_ALPHA);
  return fbos;
}

void ofApp::setup() {
  ofSetBackgroundColor(0);
  ofDisableArbTex();

  synth.configure(createFboConfigs(), createMods(), ofGetWindowSize()*8.0);
  
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
