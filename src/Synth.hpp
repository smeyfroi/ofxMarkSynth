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
  void configure(std::unique_ptr<ModPtrs> modPtrsPtr);
  void update();
  void draw();
  ofParameterGroup& getParameterGroup(const std::string& groupName);

private:
  std::unique_ptr<ModPtrs> modPtrsPtr;
  ofParameterGroup parameters;

};


} // ofxMarkSynth
