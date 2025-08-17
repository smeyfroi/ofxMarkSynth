//
//  Synth.hpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#pragma once

#include "ofxGui.h"
#include "Mod.hpp"
#ifndef TARGET_OS_IOS
#include "ofxFFmpegRecorder.h"
#endif
#include "ofThread.h"


namespace ofxMarkSynth {


struct PixelsToFile : public ofThread {
  void save(const std::string& filepath_, ofPixels&& pixels_);
  void threadedFunction();
  std::string filepath;
  ofPixels pixels;
  bool isReady = true;
};


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


class Synth : public Mod {
  
public:
  Synth(const std::string& name, const ModConfig&& config);
  ~Synth();
  void configure(FboConfigPtrs&& fboConfigPtrs_, ModPtrs&& modPtrs_, glm::vec2 compositeSize_);
  void receive(int sinkId, const glm::vec4& v) override;
  void update() override;
  void draw() override;
  void drawGui();
  void setGuiSize(glm::vec2 size);
  bool keyPressed(int key) override;
  ofParameterGroup& getFboParameterGroup();
  
  static constexpr int SINK_BACKGROUND_COLOR = 100;

protected:
  void initParameters() override;

private:
  void updateSidePanels();
  float leftSidePanelLastUpdate { 0.0 };
  float rightSidePanelLastUpdate { 0.0 };
  float leftSidePanelTimeoutSecs { 8.0 };
  float rightSidePanelTimeoutSecs { 10.0 };

  void drawSidePanels();
  PingPongFbo leftCompositeFbo, rightCompositeFbo;
  float compositeScale, sidePanelWidth, sidePanelHeight;

  ModPtrs modPtrs;
  FboConfigPtrs fboConfigPtrs;
  ofParameterGroup fboParameters;
  std::vector<std::shared_ptr<ofParameter<float>>> fboParamPtrs;
  ofParameter<ofFloatColor> backgroundColorParameter { "background color", ofFloatColor { 0.0, 0.0, 0.0, 1.0 }, ofFloatColor { 0.0, 0.0, 0.0, 1.0 }, ofFloatColor { 1.0, 1.0, 1.0, 1.0 } };

#ifndef TARGET_OS_IOS
  ofxFFmpegRecorder recorder;
  ofFbo recorderCompositeFbo;
#endif
  ofFbo imageCompositeFbo;
  
  bool guiVisible { true };
  ofxPanel gui;
  bool plusKeyPressed { false };
  bool equalsKeyPressed { false };
  
  PixelsToFile pixelsToFile;
};


std::string saveFilePath(const std::string& filename);


} // ofxMarkSynth
