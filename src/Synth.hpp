//
//  Synth.hpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#pragma once

#include "ofxGui.h"
#include "Mod.hpp"
#include "ofxFFmpegRecorder.h"


namespace ofxMarkSynth {


class Synth {
  
public:
  Synth();
  ~Synth();
  void configure(std::unique_ptr<ModPtrs> modPtrsPtr, std::shared_ptr<PingPongFbo> fboPtr_);
  void update();
  void draw();
  bool keyPressed(int key);
  ofParameterGroup& getParameterGroup(const std::string& groupName);

private:
  std::unique_ptr<ModPtrs> modPtrsPtr;
  FboPtr fboPtr;
  ofParameterGroup parameters;
  
  ofxFFmpegRecorder recorder;
  ofFbo recorderCompositeFbo;
  ofFbo imageCompositeFbo;

};


} // ofxMarkSynth
