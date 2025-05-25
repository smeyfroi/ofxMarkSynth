#include "ofApp.h"

//--------------------------------------------------------------
ofxMarkSynth::ModPtrs ofApp::createMods() {
  auto mods = ofxMarkSynth::ModPtrs {};

  auto randomFloatSourceModPtr = addMod<ofxMarkSynth::RandomFloatSourceMod>(mods, "Random Radius", ofxMarkSynth::ModConfig {
    {"CreatedPerUpdate", "0.05"},
    {"Min", "0.5"},
    {"Max", "2.0"}
  }, std::pair<float, float>{0.0, 64.0}, std::pair<float, float>{0.0, 64.0});

  auto audioDataSourceModPtr = addMod<ofxMarkSynth::AudioDataSourceMod>(mods, "Audio Points", {
    {"MinPitch", "50.0"},
    {"MaxPitch", "1500.0"}
  }, audioDataProcessorPtr);

  auto clusterModPtr = addMod<ofxMarkSynth::ClusterMod>(mods, "Clusters", {});
  audioDataSourceModPtr->addSink(ofxMarkSynth::AudioDataSourceMod::SOURCE_PITCH_RMS_POINTS,
                                 clusterModPtr,
                                 ofxMarkSynth::ClusterMod::SINK_VEC2);

  auto sandLineModPtr = addMod<ofxMarkSynth::SandLineMod>(mods, "Sand lines", {
    {"Color", "1.0, 0.0, 0.5, 0.2"},
    {"Density", "0.4"}
  });
  randomFloatSourceModPtr->addSink(ofxMarkSynth::RandomFloatSourceMod::SOURCE_FLOAT,
                                   sandLineModPtr,
                                   ofxMarkSynth::SandLineMod::SINK_POINT_RADIUS);
  clusterModPtr->addSink(ofxMarkSynth::ClusterMod::SOURCE_VEC2,
                                 sandLineModPtr,
                                 ofxMarkSynth::SandLineMod::SINK_POINTS);

  sandLineModPtr->receive(ofxMarkSynth::DrawPointsMod::SINK_FBO, fboPtr);

  return mods;
}

ofxMarkSynth::FboConfigPtrs ofApp::createFboConfigs() {
  ofxMarkSynth::FboConfigPtrs fbos;
  auto fboConfigPtrBackground = std::make_shared<ofxMarkSynth::FboConfig>(fboPtr, nullptr);
  fbos.emplace_back(fboConfigPtrBackground);
  return fbos;
}

void ofApp::setup() {
  ofSetBackgroundColor(0);
  ofDisableArbTex();

  audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>();
  audioDataProcessorPtr = std::make_shared<ofxAudioData::Processor>(audioAnalysisClientPtr);

  ofxMarkSynth::allocateFbo(fboPtr, ofGetWindowSize(), GL_RGBA32F);
  synth.configure(createMods(), createFboConfigs(), ofGetWindowSize());

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
