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


struct FboConfig {
  std::string name;
  std::shared_ptr<PingPongFbo> fboPtr;
  ofFloatColor clearColor;
  bool clearOnUpdate;
  ofBlendMode blendMode;
};

using FboConfigPtr = std::shared_ptr<FboConfig>;
using FboConfigPtrs = std::vector<FboConfigPtr>;

// Enable setting the GL wrap mode easily
void allocateFbo(FboPtr fboPtr, glm::vec2 size, GLint internalFormat, int wrap = GL_CLAMP_TO_EDGE); // GL_REPEAT
void addFboConfigPtr(FboConfigPtrs& fboConfigPtrs, std::string name, FboPtr fboPtr, glm::vec2 size, GLint internalFormat, int wrap, ofFloatColor clearColor, bool clearOnUpdate, ofBlendMode blendMode);


class Synth {
  
public:
  Synth(std::string name = "Synth");
  ~Synth();
  void configure(FboConfigPtrs&& fboConfigPtrs_, ModPtrs&& modPtrs_, glm::vec2 compositeSize_);
  void update();
  void draw();
  void drawGui();
  void setGuiSize(glm::vec2 size);
  bool keyPressed(int key);
  ofParameterGroup& getFboParameterGroup();
  ofParameterGroup& getParameterGroup(const std::string& groupName);
  void minimizeAllGuiGroupsRecursive(ofxGuiGroup& guiGroup);
  std::string name;

private:
  ModPtrs modPtrs;
  FboConfigPtrs fboConfigPtrs;
  ofParameterGroup parameters;
  ofParameterGroup fboParameters;
  std::vector<std::shared_ptr<ofParameter<float>>> fboParamPtrs;
  ofParameter<ofFloatColor> backgroundColorParameter { "background color", ofFloatColor { 0.0, 0.0, 0.0, 1.0 }, ofFloatColor { 0.0, 0.0, 0.0, 1.0 }, ofFloatColor { 1.0, 1.0, 1.0, 1.0 } };
  
  ofxFFmpegRecorder recorder;
  ofFbo recorderCompositeFbo;
  ofFbo imageCompositeFbo;
  
  bool guiVisible { true };
  ofxPanel gui;
  bool plusKeyPressed { false };
  bool equalsKeyPressed { false };
};


std::string saveFilePath(std::string filename);


} // ofxMarkSynth
