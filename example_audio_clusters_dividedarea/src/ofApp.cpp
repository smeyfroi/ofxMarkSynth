#include "ofApp.h"

//--------------------------------------------------------------
ofxMarkSynth::ModPtrs ofApp::createMods() {
  auto mods = ofxMarkSynth::ModPtrs {};

  auto audioDataSourceModPtr = std::make_shared<ofxMarkSynth::AudioDataSourceMod>("Audio Points",
                                                                                  ofxMarkSynth::ModConfig {
    {"MinPitch", "50.0"},
    {"MaxPitch", "2500.0"}
  });
  audioDataSourceModPtr->audioDataProcessorPtr = audioDataProcessorPtr;
  mods.push_back(audioDataSourceModPtr);
  
  auto clusterModPtr = std::make_shared<ofxMarkSynth::ClusterMod>("Clusters",
                                                                  ofxMarkSynth::ModConfig {});
  audioDataSourceModPtr->addSink(ofxMarkSynth::AudioDataSourceMod::SOURCE_PITCH_RMS_POINTS,
                                 clusterModPtr,
                                 ofxMarkSynth::ClusterMod::SINK_VEC2);
  mods.push_back(clusterModPtr);
  
  {
    ofxMarkSynth::ModPtr drawPointsModPtr = std::make_shared<ofxMarkSynth::DrawPointsMod>("Draw Points",
                                                                                          ofxMarkSynth::ModConfig {});
    clusterModPtr->addSink(ofxMarkSynth::ClusterMod::SOURCE_VEC2,
                           drawPointsModPtr,
                           ofxMarkSynth::DrawPointsMod::SINK_POINTS);
    mods.push_back(drawPointsModPtr);
    
    ofxMarkSynth::ModPtr multiplyModPtr = std::make_shared<ofxMarkSynth::MultiplyMod>("Fade Points",
                                                                                      ofxMarkSynth::ModConfig {});
    drawPointsModPtr->addSink(ofxMarkSynth::DrawPointsMod::SOURCE_FBO,
                              multiplyModPtr,
                              ofxMarkSynth::MultiplyMod::SINK_FBO);
    mods.push_back(multiplyModPtr);

    drawPointsModPtr->receive(ofxMarkSynth::DrawPointsMod::SINK_FBO, fboPtrBackground);
  }

  {
    ofxMarkSynth::ModPtr dividedAreaModPtr = std::make_shared<ofxMarkSynth::DividedAreaMod>("Divided Area",
                                                                                            ofxMarkSynth::ModConfig {});
    clusterModPtr->addSink(ofxMarkSynth::ClusterMod::SOURCE_VEC2,
                           dividedAreaModPtr,
                           ofxMarkSynth::DividedAreaMod::SINK_MAJOR_ANCHORS);
    clusterModPtr->addSink(ofxMarkSynth::ClusterMod::SOURCE_VEC2,
                           dividedAreaModPtr,
                           ofxMarkSynth::DividedAreaMod::SINK_MINOR_ANCHORS);
    mods.push_back(dividedAreaModPtr);
    
    ofxMarkSynth::ModPtr multiplyModPtr = std::make_shared<ofxMarkSynth::MultiplyMod>("Fade Unconstrained Lines",
                                                                                      ofxMarkSynth::ModConfig {});
    dividedAreaModPtr->addSink(ofxMarkSynth::DrawPointsMod::SOURCE_FBO_2, // Fade unconstrained lines
                              multiplyModPtr,
                              ofxMarkSynth::MultiplyMod::SINK_FBO);
    mods.push_back(multiplyModPtr);

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
