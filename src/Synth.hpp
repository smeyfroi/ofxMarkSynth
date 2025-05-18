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


// Enable setting the GL wrap mode easily
void allocateFbo(FboPtr fboPtr, glm::vec2 size, GLint internalFormat, int wrap = GL_CLAMP_TO_EDGE); // GL_REPEAT

struct FboConfig {
  std::shared_ptr<PingPongFbo> fboPtr;
  std::unique_ptr<ofFloatColor> clearColorPtr;
};

using FboConfigPtr = std::shared_ptr<FboConfig>;
using FboConfigPtrs = std::vector<FboConfigPtr>;


class Synth {
  
public:
  Synth();
  ~Synth();
  void configure(ModPtrs&& modPtrs_, FboConfigPtrs&& fboConfigPtrs_, glm::vec2 compositeSize_);
  void update();
  void draw();
  bool keyPressed(int key);
  ofParameterGroup& getParameterGroup(const std::string& groupName);

private:
  ModPtrs modPtrs;
  FboConfigPtrs fboConfigPtrs;
  ofParameterGroup parameters;
  
  ofxFFmpegRecorder recorder;
  ofFbo recorderCompositeFbo;
  ofFbo imageCompositeFbo;

};


} // ofxMarkSynth
