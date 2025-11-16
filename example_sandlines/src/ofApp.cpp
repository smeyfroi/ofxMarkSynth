#include "ofApp.h"
#include "ofxTimeMeasurements.h"
#include "ModFactory.hpp"

//--------------------------------------------------------------
//ofxMarkSynth::ModPtrs ofApp::createMods() {
//  auto mods = ofxMarkSynth::ModPtrs {};
//
//  auto randomFloatSourceModPtr = addMod<ofxMarkSynth::RandomFloatSourceMod>(mods, "Random Radius", ofxMarkSynth::ModConfig {
//    {"CreatedPerUpdate", "0.05"},
//    {"Min", "0.5"},
//    {"Max", "2.0"}
//  }, std::pair<float, float>{0.0, 64.0}, std::pair<float, float>{0.0, 64.0});
//
//  auto audioDataSourceModPtr = addMod<ofxMarkSynth::AudioDataSourceMod>(mods, "Audio Points", {
//    {"MinPitch", "50.0"},
//    {"MaxPitch", "1500.0"}
//  }, audioDataProcessorPtr);
//
//  auto clusterModPtr = addMod<ofxMarkSynth::ClusterMod>(mods, "Clusters", {});
//  audioDataSourceModPtr->addSink(ofxMarkSynth::AudioDataSourceMod::SOURCE_PITCH_RMS_POINTS,
//                                 clusterModPtr,
//                                 ofxMarkSynth::ClusterMod::SINK_VEC2);
//
//  auto sandLineModPtr = addMod<ofxMarkSynth::SandLineMod>(mods, "Sand lines", {
//    {"Color", "1.0, 0.0, 0.5, 0.2"},
//    {"Density", "0.4"}
//  });
//  randomFloatSourceModPtr->addSink(ofxMarkSynth::RandomFloatSourceMod::SOURCE_FLOAT,
//                                   sandLineModPtr,
//                                   ofxMarkSynth::SandLineMod::SINK_POINT_RADIUS);
//  clusterModPtr->addSink(ofxMarkSynth::ClusterMod::SOURCE_VEC2,
//                                 sandLineModPtr,
//                                 ofxMarkSynth::SandLineMod::SINK_POINTS);
//
//  sandLineModPtr->receive(ofxMarkSynth::DrawPointsMod::SINK_FBO, fboPtr);
//
//  return mods;
//}

//ofxMarkSynth::FboConfigPtrs ofApp::createFboConfigs() {
//  ofxMarkSynth::FboConfigPtrs fbos;
//  auto fboConfigPtrBackground = std::make_shared<ofxMarkSynth::FboConfig>(fboPtr, nullptr);
//  fbos.emplace_back(fboConfigPtrBackground);
//  return fbos;
//}

void ofApp::setup() {
  ofDisableArbTex();
  glEnable(GL_PROGRAM_POINT_SIZE);
  ofSetBackgroundColor(0);
  ofSetFrameRate(FRAME_RATE);
  TIME_SAMPLE_SET_FRAMERATE(FRAME_RATE);
  
  ofxMarkSynth::ResourceManager resources;
  resources.add("sourceVideoPath", SOURCE_VIDEO_PATH);
  resources.add("sourceVideoMute", SOURCE_VIDEO_MUTE);
  resources.add("sourceAudioPath", SOURCE_AUDIO_PATH);
  resources.add("micDeviceName", MIC_DEVICE_NAME);
  resources.add("recordAudio", RECORD_AUDIO);
  resources.add("recordingPath", RECORDING_PATH);

  synthPtr = std::make_shared<ofxMarkSynth::Synth>("Fade", ofxMarkSynth::ModConfig {
  }, START_PAUSED, SYNTH_COMPOSITE_SIZE);

  synthPtr->loadFromConfig(ofToDataPath("2.json"), resources);
  synthPtr->configureGui(nullptr); // nullptr == no imgui window

  // No imgui; we manage an ofxGui here instead
  parameters.add(synthPtr->getParameterGroup());
  gui.setup(parameters);
}

//--------------------------------------------------------------
void ofApp::update(){
  synthPtr->update();
}

//--------------------------------------------------------------
void ofApp::draw(){
  synthPtr->draw();
  if (guiVisible) gui.draw();
}

//--------------------------------------------------------------
void ofApp::exit(){
  synthPtr->shutdown();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
  if (key == OF_KEY_TAB) guiVisible = not guiVisible;
  if (synthPtr->keyPressed(key)) return;
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
