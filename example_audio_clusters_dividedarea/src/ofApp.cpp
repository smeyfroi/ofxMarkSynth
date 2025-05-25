#include "ofApp.h"

//--------------------------------------------------------------
ofxMarkSynth::ModPtrs ofApp::createMods() {
  auto mods = ofxMarkSynth::ModPtrs {};

  auto audioDataSourceModPtr = addMod<ofxMarkSynth::AudioDataSourceMod>(mods, "Audio Points", {
    {"MinPitch", "50.0"},
    {"MaxPitch", "2500.0"}
  }, audioDataProcessorPtr);

  auto clusterModPtr = addMod<ofxMarkSynth::ClusterMod>(mods, "Clusters", {});
  audioDataSourceModPtr->addSink(ofxMarkSynth::AudioDataSourceMod::SOURCE_PITCH_RMS_POINTS,
                                 clusterModPtr,
                                 ofxMarkSynth::ClusterMod::SINK_VEC2);

  {
    auto drawPointsModPtr = addMod<ofxMarkSynth::DrawPointsMod>(mods, "Draw Points", {});
    clusterModPtr->addSink(ofxMarkSynth::ClusterMod::SOURCE_VEC2,
                           drawPointsModPtr,
                           ofxMarkSynth::DrawPointsMod::SINK_POINTS);

    auto multiplyModPtr = addMod<ofxMarkSynth::MultiplyMod>(mods, "Fade Points", {});
    drawPointsModPtr->addSink(ofxMarkSynth::DrawPointsMod::SOURCE_FBO,
                              multiplyModPtr,
                              ofxMarkSynth::MultiplyMod::SINK_FBO);

    drawPointsModPtr->receive(ofxMarkSynth::DrawPointsMod::SINK_FBO, fboPtrBackground);
  }

  {
    auto dividedAreaModPtr = addMod<ofxMarkSynth::DividedAreaMod>(mods, "Divided Area", {});
    clusterModPtr->addSink(ofxMarkSynth::ClusterMod::SOURCE_VEC2,
                           dividedAreaModPtr,
                           ofxMarkSynth::DividedAreaMod::SINK_MAJOR_ANCHORS);
    clusterModPtr->addSink(ofxMarkSynth::ClusterMod::SOURCE_VEC2,
                           dividedAreaModPtr,
                           ofxMarkSynth::DividedAreaMod::SINK_MINOR_ANCHORS);

    auto multiplyModPtr = addMod<ofxMarkSynth::MultiplyMod>(mods, "Fade Unconstrained Lines", {});
    dividedAreaModPtr->addSink(ofxMarkSynth::DrawPointsMod::SOURCE_FBO_2, // Fade unconstrained lines
                              multiplyModPtr,
                              ofxMarkSynth::MultiplyMod::SINK_FBO);

    dividedAreaModPtr->receive(ofxMarkSynth::DividedAreaMod::SINK_FBO, fboPtrMinorLines);
    dividedAreaModPtr->receive(ofxMarkSynth::DividedAreaMod::SINK_FBO_2, fboPtrMajorLines);
  }
  return mods;
}

ofxMarkSynth::FboConfigPtrs ofApp::createFboConfigs() {
  ofxMarkSynth::FboConfigPtrs fbos;
  auto fboConfigPtrBackground = std::make_shared<ofxMarkSynth::FboConfig>(fboPtrBackground, nullptr);
  fbos.emplace_back(fboConfigPtrBackground);
  auto fboConfigPtrMinorLines = std::make_shared<ofxMarkSynth::FboConfig>(fboPtrMinorLines, std::make_unique<ofFloatColor>(0.0, 0.0, 0.0, 1.0));
  fbos.emplace_back(fboConfigPtrMinorLines);
  auto fboConfigPtrMajorLines = std::make_shared<ofxMarkSynth::FboConfig>(fboPtrMajorLines, nullptr);
  fbos.emplace_back(fboConfigPtrMajorLines);
  return fbos;
}

void ofApp::setup() {
  ofSetBackgroundColor(0);
  ofDisableArbTex();

  const std::filesystem::path rootSourceMaterialPath { "/Users/steve/Documents/music-source-material" };
  //  audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>(rootSourceMaterialPath/"20250208-trombone-melody.wav");
  audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>();
  audioDataProcessorPtr = std::make_shared<ofxAudioData::Processor>(audioAnalysisClientPtr);

  ofxMarkSynth::allocateFbo(fboPtrBackground, ofGetWindowSize(), GL_RGBA32F);
  ofxMarkSynth::allocateFbo(fboPtrMinorLines, ofGetWindowSize(), GL_RGBA);
  ofxMarkSynth::allocateFbo(fboPtrMajorLines, ofGetWindowSize(), GL_RGBA32F);
  synth.configure(createMods(), createFboConfigs(), ofGetWindowSize());

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
