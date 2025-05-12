//
//  Synth.hpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#pragma once

#include "ofxGui.h"
#include "Mod.hpp"


namespace ofxMarkSynth {


class Synth {
  
public:
  void configure(std::unique_ptr<ModPtrs> modPtrsPtr, std::shared_ptr<PingPongFbo> fboPtr_);
  void update();
  void draw();
  bool keyPressed(int key);
  ofParameterGroup& getParameterGroup(const std::string& groupName);

private:
  std::unique_ptr<ModPtrs> modPtrsPtr;
  FboPtr fboPtr;
  ofParameterGroup parameters;
};


} // ofxMarkSynth
