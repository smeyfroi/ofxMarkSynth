#include "ofApp.h"

//--------------------------------------------------------------
std::unique_ptr<ofxMarkSynth::ModPtrs> ofApp::createMods() {
  auto mods = std::make_unique<ofxMarkSynth::ModPtrs>();

  auto randomFloatSourceModPtr = std::make_shared<ofxMarkSynth::RandomFloatSourceMod>("Random Radius", ofxMarkSynth::ModConfig {
    {"CreatedPerUpdate", "0.05"},
    {"Min", "1.0"},
    {"Max", "5.0"}
  }, std::pair<float, float>{0.0, 64.0}, std::pair<float, float>{0.0, 64.0});
  mods->push_back(randomFloatSourceModPtr);

  auto randomVecSourceModPtr = std::make_shared<ofxMarkSynth::RandomVecSourceMod>("Random Points", ofxMarkSynth::ModConfig {
    {"CreatedPerUpdate", "0.4"}
  }, 2);
  mods->push_back(randomVecSourceModPtr);
  
  auto randomColourSourceModPtr = std::make_shared<ofxMarkSynth::RandomVecSourceMod>("Random Colours", ofxMarkSynth::ModConfig {
    {"CreatedPerUpdate", "0.1"}
  }, 4);
  mods->push_back(randomColourSourceModPtr);
  
  auto drawPointsModPtr = std::make_shared<ofxMarkSynth::DrawPointsMod>("Draw Points", ofxMarkSynth::ModConfig {
  }, ofGetWindowSize());
  randomColourSourceModPtr->addSink(ofxMarkSynth::RandomVecSourceMod::SOURCE_VEC4,
                                   drawPointsModPtr,
                                   ofxMarkSynth::DrawPointsMod::SINK_POINT_COLOR);
  randomFloatSourceModPtr->addSink(ofxMarkSynth::RandomFloatSourceMod::SOURCE_FLOAT,
                                   drawPointsModPtr,
                                   ofxMarkSynth::DrawPointsMod::SINK_POINT_RADIUS);
  randomVecSourceModPtr->addSink(ofxMarkSynth::RandomVecSourceMod::SOURCE_VEC2,
                                 drawPointsModPtr,
                                 ofxMarkSynth::DrawPointsMod::SINK_POINTS);
  mods->push_back(drawPointsModPtr);
  
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
