#include "ofApp.h"

//--------------------------------------------------------------
ofxMarkSynth::ModPtrs ofApp::createMods() {
  auto mods = ofxMarkSynth::ModPtrs {};

  auto randomFloatSourceModPtr = std::make_shared<ofxMarkSynth::RandomFloatSourceMod>("Random Radius", ofxMarkSynth::ModConfig {
    {"CreatedPerUpdate", "0.05"},
    {"Min", "0.5"},
    {"Max", "2.0"}
  }, std::pair<float, float>{0.0, 64.0}, std::pair<float, float>{0.0, 64.0});
  mods.push_back(randomFloatSourceModPtr);

  auto audioDataSourceModPtr = std::make_shared<ofxMarkSynth::AudioDataSourceMod>("Audio Points",
                                                                                  ofxMarkSynth::ModConfig {
    {"MinPitch", "50.0"},
    {"MaxPitch", "1500.0"}
  });
  audioDataSourceModPtr->audioDataProcessorPtr = audioDataProcessorPtr;
  mods.push_back(audioDataSourceModPtr);
  
  auto clusterModPtr = std::make_shared<ofxMarkSynth::ClusterMod>("Clusters",
                                                                  ofxMarkSynth::ModConfig {
  });
  audioDataSourceModPtr->addSink(ofxMarkSynth::AudioDataSourceMod::SOURCE_PITCH_RMS_POINTS,
                                 clusterModPtr,
                                 ofxMarkSynth::ClusterMod::SINK_VEC2);
  mods.push_back(clusterModPtr);

  ofxMarkSynth::ModPtr sandLineModPtr = std::make_shared<ofxMarkSynth::SandLineMod>("Sand lines", ofxMarkSynth::ModConfig {
    {"Color", "1.0, 0.0, 0.5, 0.2"},
    {"Density", "0.4"}
  }, ofGetWindowSize());
  randomFloatSourceModPtr->addSink(ofxMarkSynth::RandomFloatSourceMod::SOURCE_FLOAT,
                                   sandLineModPtr,
                                   ofxMarkSynth::SandLineMod::SINK_POINT_RADIUS);
  clusterModPtr->addSink(ofxMarkSynth::ClusterMod::SOURCE_VEC2,
                                 sandLineModPtr,
                                 ofxMarkSynth::SandLineMod::SINK_POINTS);
  mods.push_back(sandLineModPtr);
  
  sandLineModPtr->receive(ofxMarkSynth::DrawPointsMod::SINK_FBO, fboPtr);

  return mods;
}

void ofApp::setup() {
  ofSetBackgroundColor(0);
  ofDisableArbTex();

  audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>();
  audioDataProcessorPtr = std::make_shared<ofxAudioData::Processor>(audioAnalysisClientPtr);

  fboPtr->allocate(ofGetWindowWidth(), ofGetWindowHeight(), GL_RGBA32F);
  fboPtr->getSource().clearColorBuffer(ofFloatColor(0.0, 0.0, 0.0, 0.0));
  synth.configure(createMods(), fboPtr);
  
  parameters.add(synth.getParameterGroup("Synth"));
  gui.setup(parameters);
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
