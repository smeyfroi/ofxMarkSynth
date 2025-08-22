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
#include "TonemapShader.h"


namespace ofxMarkSynth {


struct SaveToFileThread : public ofThread {
  void save(const std::string& filepath_, ofFloatPixels&& pixels_);
  void threadedFunction();
  std::string filepath;
  ofFloatPixels pixels;
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
  void shutdown();
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
  float leftSidePanelTimeoutSecs { 7.0 };
  float rightSidePanelTimeoutSecs { 11.0 };

  void drawSidePanels();
  PingPongFbo leftPanelFbo, rightPanelFbo;
  ofFbo leftPanelCompositeFbo, rightPanelCompositeFbo;
  float compositeScale, sidePanelWidth, sidePanelHeight;

  ModPtrs modPtrs;
  FboConfigPtrs fboConfigPtrs;
  ofParameterGroup fboParameters;
  std::vector<std::shared_ptr<ofParameter<float>>> fboParamPtrs;
  
  ofParameterGroup displayParameters;
  ofParameter<ofFloatColor> backgroundColorParameter { "background color", ofFloatColor { 0.0, 0.0, 0.0, 1.0 }, ofFloatColor { 0.0, 0.0, 0.0, 1.0 }, ofFloatColor { 1.0, 1.0, 1.0, 1.0 } };
  ofParameter<float> backgroundMultiplierParameter { "backgroundMultiplier", 0.1, 0.0, 1.0 };
  ofParameter<int> toneMapTypeParameter { "tone map type", 3, 0, 5 }; // 0: Linear (clamp); 1: Reinhard; 2: Reinhard Extended; 3: ACES; 4: Filmic; 5: Exposure
  ofParameter<float> exposureParameter { "exposure", 1.0, 0.0, 4.0 };
  ofParameter<float> gammaParameter { "gamma", 1.8, 0.1, 5.0 }; // 2.2
  ofParameter<float> whitePointParameter { "white point", 11.2, 1.0, 20.0 }; // for Reinhard Extended
  ofParameter<float> sideExposureParameter { "sideExposure", 0.3, 0.0, 4.0 };

#ifndef TARGET_OS_IOS
  ofxFFmpegRecorder recorder;
  ofFbo recorderCompositeFbo;
#endif
  ofFbo imageCompositeFbo;
  TonemapShader tonemapShader;
  
  bool guiVisible { true };
  ofxPanel gui;
  bool plusKeyPressed { false };
  bool equalsKeyPressed { false };
  
  std::vector<SaveToFileThread*> saveToFileThreads;
};


std::string saveFilePath(const std::string& filename);


} // ofxMarkSynth
