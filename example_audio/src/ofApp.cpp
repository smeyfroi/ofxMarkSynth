#include "ofApp.h"

//--------------------------------------------------------------
std::unique_ptr<ofxMarkSynth::ModPtrs> ofApp::createMods() {
  auto mods = std::make_unique<ofxMarkSynth::ModPtrs>();

  auto audioDataSourceModPtr = std::make_shared<ofxMarkSynth::AudioDataSourceMod>("Audio Points",
                                                                                  ofxMarkSynth::ModConfig {
    {"MinPitch", "50.0"},
    {"MaxPitch", "2500.0"}
  });
  audioDataSourceModPtr->audioDataProcessorPtr = audioDataProcessorPtr;
  mods->push_back(audioDataSourceModPtr);
  
  auto pointIntrospectorModPtr = std::make_shared<ofxMarkSynth::PointIntrospectorMod>("Point Introspector",
                                                                                      ofxMarkSynth::ModConfig {
  });
  pointIntrospectorModPtr->introspectorPtr = introspectorPtr;
  audioDataSourceModPtr->addSink(ofxMarkSynth::AudioDataSourceMod::SOURCE_PITCH_RMS_POINTS,
                                 pointIntrospectorModPtr,
                                 ofxMarkSynth::PointIntrospectorMod::SINK_POINTS);
  mods->push_back(pointIntrospectorModPtr);
  
  return mods;
}

void ofApp::setup() {
  ofSetBackgroundColor(0);
  
  introspectorPtr = std::make_shared<Introspector>();
  introspectorPtr->visible = true;
  
  audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>();
  audioDataProcessorPtr = std::make_shared<ofxAudioData::Processor>(audioAnalysisClientPtr);
  
  synth.configure(createMods());
  
  parameters.add(synth.getParameterGroup("Synth"));
  gui.setup(parameters);
}

//--------------------------------------------------------------
void ofApp::update(){
  introspectorPtr->update();
  audioDataProcessorPtr->update();
  synth.update();
}

//--------------------------------------------------------------
void ofApp::draw(){
  synth.draw();
  introspectorPtr->draw(ofGetWindowWidth());

  if (guiVisible) gui.draw();
}

//--------------------------------------------------------------
void ofApp::exit(){
  audioAnalysisClientPtr->closeStream();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
  if (key == OF_KEY_TAB) guiVisible = not guiVisible;
  if (introspectorPtr->keyPressed(key)) return;
  if (audioAnalysisClientPtr->keyPressed(key)) return;
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
