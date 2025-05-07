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
  
  auto clusterModPtr = std::make_shared<ofxMarkSynth::ClusterMod>("Clusters",
                                                                  ofxMarkSynth::ModConfig {
  });
  audioDataSourceModPtr->addSink(ofxMarkSynth::AudioDataSourceMod::SOURCE_PITCH_RMS_POINTS,
                                 clusterModPtr,
                                 ofxMarkSynth::ClusterMod::SINK_VEC2);
  mods->push_back(clusterModPtr);
  
  auto drawPointsModPtr = std::make_shared<ofxMarkSynth::DrawPointsMod>("Draw Points",
                                                                        ofxMarkSynth::ModConfig {
  }, ofGetWindowSize());
  clusterModPtr->addSink(ofxMarkSynth::ClusterMod::SOURCE_VEC2,
                         drawPointsModPtr,
                         ofxMarkSynth::DrawPointsMod::SINK_POINTS);
  mods->push_back(drawPointsModPtr);
  
  return mods;
}

void ofApp::setup() {
  ofSetBackgroundColor(0);
  ofDisableArbTex();

//  const std::filesystem::path rootSourceMaterialPath { "/Users/steve/Documents/music-source-material" };
  //  audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>(rootSourceMaterialPath/"20250208-trombone-melody.wav");
  audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>();
  audioDataProcessorPtr = std::make_shared<ofxAudioData::Processor>(audioAnalysisClientPtr);

  synth.configure(createMods());
  
  parameters.add(synth.getParameterGroup("Synth"));
  gui.setup(parameters);
  gui.getGroup("Synth").minimizeAll();
}

//--------------------------------------------------------------
void ofApp::update(){
  audioDataProcessorPtr->update();
  synth.update();
}

//--------------------------------------------------------------
void ofApp::draw(){
  synth.draw();
  if (guiVisible) gui.draw();
}

//--------------------------------------------------------------
void ofApp::exit(){
  audioAnalysisClientPtr->closeStream();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
  if (key == OF_KEY_TAB) guiVisible = not guiVisible;
  if (audioAnalysisClientPtr->keyPressed(key)) return;
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
