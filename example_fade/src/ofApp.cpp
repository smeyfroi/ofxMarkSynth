#include "ofApp.h"

//--------------------------------------------------------------
std::unique_ptr<ofxMarkSynth::ModPtrs> ofApp::createMods() {
  auto mods = std::make_unique<ofxMarkSynth::ModPtrs>();

  auto randomVecSourceModPtr = std::make_shared<ofxMarkSynth::RandomVecSourceMod>("Random Points", ofxMarkSynth::ModConfig {
    {"CreatedPerUpdate", "0.4"}
  }, 2);
  mods->push_back(randomVecSourceModPtr);
  
  auto drawPointsModPtr = std::make_shared<ofxMarkSynth::DrawPointsMod>("Draw Points", ofxMarkSynth::ModConfig {
  }, ofGetWindowSize());
  randomVecSourceModPtr->addSink(ofxMarkSynth::RandomVecSourceMod::SOURCE_VEC2,
                                 drawPointsModPtr,
                                 ofxMarkSynth::DrawPointsMod::SINK_POINTS);
  mods->push_back(drawPointsModPtr);
  
  auto translateModPtr = std::make_shared<ofxMarkSynth::TranslateMod>("Translate", ofxMarkSynth::ModConfig {
  });
  drawPointsModPtr->addSink(ofxMarkSynth::DrawPointsMod::SOURCE_FBO,
                            translateModPtr,
                            ofxMarkSynth::TranslateMod::SINK_FBO);
  mods->push_back(translateModPtr);
  
  auto multiplyModPtr = std::make_shared<ofxMarkSynth::MultiplyMod>("Fade", ofxMarkSynth::ModConfig {
  });
  translateModPtr->addSink(ofxMarkSynth::TranslateMod::SOURCE_FBO,
                           multiplyModPtr,
                           ofxMarkSynth::MultiplyMod::SINK_FBO);
  mods->push_back(multiplyModPtr);
  
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
