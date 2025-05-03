#include "ofApp.h"

//--------------------------------------------------------------
std::unique_ptr<ofxMarkSynth::ModPtrs> ofApp::createMods() {
  auto mods = std::make_unique<ofxMarkSynth::ModPtrs>();

  auto randomPointsSourceModPtr = std::make_shared<ofxMarkSynth::RandomPointSourceMod>("Random Points", ofxMarkSynth::ModConfig {
    {"PointsPerUpdate", "0.4"}
  });
  mods->push_back(randomPointsSourceModPtr);
  
  auto pointIntrospectorModPtr = std::make_shared<ofxMarkSynth::PointIntrospectorMod>("Point Introspector", ofxMarkSynth::ModConfig {
  });
  pointIntrospectorModPtr->introspectorPtr = introspectorPtr;
  randomPointsSourceModPtr->addSink(ofxMarkSynth::RandomPointSourceMod::SOURCE_POINTS,
                                    pointIntrospectorModPtr,
                                    ofxMarkSynth::PointIntrospectorMod::SINK_POINTS);
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
