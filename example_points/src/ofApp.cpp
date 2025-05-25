#include "ofApp.h"

//--------------------------------------------------------------
ofxMarkSynth::ModPtrs ofApp::createMods() {
  auto mods = ofxMarkSynth::ModPtrs {};

  auto randomFloatSourceModPtr = addMod<ofxMarkSynth::RandomFloatSourceMod>(mods, "Random Radius", {
    {"CreatedPerUpdate", "0.05"},
    {"Min", "0.001"},
    {"Max", "0.05"}
  }, std::pair<float, float>{0.0, 0.1}, std::pair<float, float>{0.0, 0.1});

  auto randomVecSourceModPtr = addMod<ofxMarkSynth::RandomVecSourceMod>(mods, "Random Points", {
    {"CreatedPerUpdate", "0.4"}
  }, 2);
  
  auto randomColourSourceModPtr = addMod<ofxMarkSynth::RandomVecSourceMod>(mods, "Random Colours", ofxMarkSynth::ModConfig {
    {"CreatedPerUpdate", "0.1"}
  }, 4);
  
  auto drawPointsModPtr = addMod<ofxMarkSynth::DrawPointsMod>(mods, "Draw Points", {});
  randomColourSourceModPtr->addSink(ofxMarkSynth::RandomVecSourceMod::SOURCE_VEC4,
                                   drawPointsModPtr,
                                   ofxMarkSynth::DrawPointsMod::SINK_POINT_COLOR);
  randomFloatSourceModPtr->addSink(ofxMarkSynth::RandomFloatSourceMod::SOURCE_FLOAT,
                                   drawPointsModPtr,
                                   ofxMarkSynth::DrawPointsMod::SINK_POINT_RADIUS);
  randomVecSourceModPtr->addSink(ofxMarkSynth::RandomVecSourceMod::SOURCE_VEC2,
                                 drawPointsModPtr,
                                 ofxMarkSynth::DrawPointsMod::SINK_POINTS);

  auto fluidModPtr = addMod<ofxMarkSynth::FluidMod>(mods, "Fluid", ofxMarkSynth::ModConfig {});

  drawPointsModPtr->receive(ofxMarkSynth::DrawPointsMod::SINK_FBO, fboPtr);
  fluidModPtr->receive(ofxMarkSynth::FluidMod::SINK_VALUES_FBO, fboPtr);
  fluidModPtr->receive(ofxMarkSynth::FluidMod::SINK_VELOCITIES_FBO, fluidVelocitiesFboPtr);
  
  return mods;
}

ofxMarkSynth::FboConfigPtrs ofApp::createFboConfigs() {
  ofxMarkSynth::FboConfigPtrs fbos;
  auto fboConfigPtrFluidValues = std::make_shared<ofxMarkSynth::FboConfig>(fboPtr, nullptr);
  fbos.emplace_back(fboConfigPtrFluidValues);
  return fbos;
}

void ofApp::setup() {
  ofSetBackgroundColor(0);
  ofDisableArbTex();
  ofSetFrameRate(60);

  fboPtr->allocate(ofGetWindowWidth(), ofGetWindowHeight(), GL_RGBA32F);
  fboPtr->getSource().clearColorBuffer(ofFloatColor(0.0, 0.0, 0.0, 0.0));
  fluidVelocitiesFboPtr->allocate(ofGetWindowWidth(), ofGetWindowHeight(), GL_RGB32F);
  fluidVelocitiesFboPtr->getSource().clearColorBuffer(ofFloatColor(0.0, 0.0, 0.0));
  synth.configure(createMods(), createFboConfigs(), ofGetWindowSize());
  
  parameters.add(synth.getParameterGroup("Synth"));
  gui.setup(parameters);
}

//--------------------------------------------------------------
void ofApp::update(){
  synth.update();
}

//--------------------------------------------------------------
void ofApp::draw(){
  synth.draw();
  if (guiVisible) gui.draw();
}

//--------------------------------------------------------------
void ofApp::exit(){
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
  if (key == OF_KEY_TAB) { guiVisible = not guiVisible; return; }
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
