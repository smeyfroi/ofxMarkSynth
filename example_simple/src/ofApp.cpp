#include "ofApp.h"

//--------------------------------------------------------------
std::unique_ptr<ofxMarkSynth::ModPtrs> ofApp::createMods() {
  auto mods = std::make_unique<ofxMarkSynth::ModPtrs>();

  auto randomVecSourceModPtr = std::make_shared<ofxMarkSynth::RandomVecSourceMod>("Random Points", ofxMarkSynth::ModConfig {
    {"CreatedPerUpdate", "0.4"}
  }, 2);
  mods->push_back(randomVecSourceModPtr);
  
  auto pointIntrospectorModPtr = std::make_shared<ofxMarkSynth::IntrospectorMod>("Introspector", ofxMarkSynth::ModConfig {
  });
  pointIntrospectorModPtr->introspectorPtr = introspectorPtr;
  randomVecSourceModPtr->addSink(ofxMarkSynth::RandomVecSourceMod::SOURCE_VEC2,
                                 pointIntrospectorModPtr,
                                 ofxMarkSynth::IntrospectorMod::SINK_POINTS);
  mods->push_back(pointIntrospectorModPtr);
  
  return mods;
}

void ofApp::setup() {
  ofSetBackgroundColor(0);
  
  introspectorPtr = std::make_shared<Introspector>();
  introspectorPtr->visible = true;
  
  synth.configure(createMods());
  
  parameters.add(synth.getParameterGroup("Synth"));
  gui.setup(parameters);
}

//--------------------------------------------------------------
void ofApp::update(){
  synth.update();
  introspectorPtr->update();
}

//--------------------------------------------------------------
void ofApp::draw(){
  synth.draw();
  introspectorPtr->draw(ofGetWindowWidth());

  if (guiVisible) gui.draw();
}

//--------------------------------------------------------------
void ofApp::exit(){
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
  if (key == OF_KEY_TAB) guiVisible = not guiVisible;
  if (introspectorPtr->keyPressed(key)) return;
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
