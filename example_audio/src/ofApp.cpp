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
  
  auto introspectorModPtr = std::make_shared<ofxMarkSynth::IntrospectorMod>("Point Introspector",
                                                                                      ofxMarkSynth::ModConfig {
  });
  introspectorModPtr->introspectorPtr = introspectorPtr;
  audioDataSourceModPtr->addSink(ofxMarkSynth::AudioDataSourceMod::SOURCE_PITCH_RMS_POINTS,
                                 introspectorModPtr,
                                 ofxMarkSynth::IntrospectorMod::SINK_POINTS);
  audioDataSourceModPtr->addSink(ofxMarkSynth::AudioDataSourceMod::SOURCE_COMPLEX_SPECTRAL_DIFFERENCE_SCALAR,
                                 introspectorModPtr,
                                 ofxMarkSynth::IntrospectorMod::SINK_HORIZONTAL_LINES_1);
  audioDataSourceModPtr->addSink(ofxMarkSynth::AudioDataSourceMod::SOURCE_SPECTRAL_CREST_SCALAR,
                                 introspectorModPtr,
                                 ofxMarkSynth::IntrospectorMod::SINK_HORIZONTAL_LINES_2);
  audioDataSourceModPtr->addSink(ofxMarkSynth::AudioDataSourceMod::SOURCE_ZERO_CROSSING_RATE_SCALAR,
                                 introspectorModPtr,
                                 ofxMarkSynth::IntrospectorMod::SINK_HORIZONTAL_LINES_3);
  mods->push_back(introspectorModPtr);
  
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
  gui.getGroup("Synth").minimizeAll();
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
