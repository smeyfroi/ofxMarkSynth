//
//  Synth.hpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#pragma once

#include "Mod.hpp"
#ifdef TARGET_MAC
#include "ofxFFmpegRecorder.h"
#endif
#include "TonemapShader.h"
#include <unordered_map>
#include "SaveToFileThread.hpp"
#include "Intent.hpp"
#include "ParamController.h"
#include "Gui.hpp"
#include "NodeEditorModel.hpp"
#include "NodeEditorLayout.hpp"
#include "LoggerChannel.hpp"



namespace ofxMarkSynth {



const ofFloatColor DEFAULT_CLEAR_COLOR { 0.0, 0.0, 0.0, 0.0 };

using DrawingLayerPtrMap = std::map<std::string, DrawingLayerPtr>;
using ModPtrMap = std::unordered_map<std::string, ofxMarkSynth::ModPtr>;



class Synth : public Mod {
  
public:
  Synth(const std::string& name, const ModConfig&& config, bool startPaused, glm::vec2 compositeSize_);
  void drawGui();
  void shutdown() override;
  
  template <typename ModT>
  ofxMarkSynth::ModPtr addMod(const std::string& name, ofxMarkSynth::ModConfig&& modConfig);
  template <typename ModT, typename... Args>
  ofxMarkSynth::ModPtr addMod(const std::string& name, ofxMarkSynth::ModConfig&& modConfig, Args&&... args);
  ofxMarkSynth::ModPtr getMod(const std::string& name) const { return modPtrs.at(name); }
  void addMod(ofxMarkSynth::ModPtr modPtr) { modPtrs.insert({ modPtr->getName(), modPtr }); }
  DrawingLayerPtr addDrawingLayer(std::string name, glm::vec2 size, GLint internalFormat, int wrap, float fadeBy, ofBlendMode blendMode, bool useStencil, int numSamples, bool isDrawn = true);
  void addConnections(const std::string& dsl);
  void configureGui(std::shared_ptr<ofAppBaseWindow> windowPtr);
  ofParameterGroup& getIntentParameterGroup() { return intentParameters; }
  void addLiveTexturePtrFn(std::string name, std::function<const ofTexture*()> textureAccessor);

  float getAgency() const override { return agencyParameter; }

  void receive(int sinkId, const glm::vec4& v) override;
  void receive(int sinkId, const float& v) override;
  void update() override;
  glm::vec2 getSize() const { return compositeSize; }
  void draw() override;

  void audioCallback(float* buffer, int bufferSize, int nChannels);

  void toggleRecording();
  void saveImage();
  bool keyPressed(int key) override;
  
  static constexpr int SOURCE_COMPOSITE_FBO = 1;
  static constexpr int SINK_BACKGROUND_COLOR = 100;
  static constexpr int SINK_RESET_RANDOMNESS = 200;
  
  friend class Gui;
  friend class NodeEditorModel;
  friend class NodeEditorLayout;
  
protected:
  void initParameters() override;

private:
  ModPtrMap modPtrs;
  DrawingLayerPtrMap drawingLayerPtrs;
  Gui gui;
  void initDisplayParameterGroup();
  void initFboParameterGroup();
  void initIntentParameterGroup();
  std::map<std::string, std::function<const ofTexture*()>> liveTexturePtrFns; // named accessors for GUI

  bool paused;
  
  TonemapShader tonemapShader;
  glm::vec2 compositeSize;
  float compositeScale;
  ofFbo imageCompositeFbo;
  void updateCompositeImage();
  
  float sidePanelWidth, sidePanelHeight;
  PingPongFbo leftPanelFbo, rightPanelFbo;
  ofFbo leftPanelCompositeFbo, rightPanelCompositeFbo;
  void updateCompositeSideImages();
  
  float leftSidePanelLastUpdate { 0.0 };
  float rightSidePanelLastUpdate { 0.0 };
  float leftSidePanelTimeoutSecs { 7.0 };
  float rightSidePanelTimeoutSecs { 11.0 };
  void updateSidePanels();

  void drawMiddlePanel(float w, float h, float scale);
  void drawSidePanels(float xleft, float xright, float w, float h);
  void drawDebugViews();

  // Intent system
  IntentActivations intentActivations;
  Intent activeIntent { "Active", 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };
  ofParameterGroup intentParameters;
  ofParameter<float> intentStrengthParameter { "Intent Strength", 0.0, 0.0, 1.0 };
  std::vector<std::shared_ptr<ofParameter<float>>> intentActivationParameters;
  ofxLabel activeIntentInfoLabel1, activeIntentInfoLabel2;
  const Intent& getActiveIntent() const { return activeIntent; }
  float getIntentStrength() const { return intentStrengthParameter; }
  void applyIntent(const Intent& intent, float intentStrength) override;
  void initIntentPresets();
  void updateIntentActivations();
  void computeActiveIntent();
  void applyIntentToAllMods();
  
  ofParameter<float> agencyParameter { "Synth Agency", 0.0, 0.0, 1.0 }; // 0.0 -> fully manual; 1.0 -> fully autonomous
  ofParameterGroup fboParameters;
  std::vector<std::shared_ptr<ofParameter<float>>> fboParamPtrs;
  ofParameterGroup displayParameters;
  ofParameter<ofFloatColor> backgroundColorParameter { "Back Color", ofFloatColor { 0.0, 0.0, 0.0, 1.0 }, ofFloatColor { 0.0, 0.0, 0.0, 1.0 }, ofFloatColor { 1.0, 1.0, 1.0, 1.0 } };
  ParamController<ofFloatColor> backgroundColorController { backgroundColorParameter };
  ofParameter<float> backgroundMultiplierParameter { "Back Mult", 0.1, 0.0, 1.0 };
  ofParameter<int> toneMapTypeParameter { "Tone map", 3, 0, 5 }; // 0: Linear (clamp); 1: Reinhard; 2: Reinhard Extended; 3: ACES; 4: Filmic; 5: Exposure
  ofParameter<float> exposureParameter { "Exposure", 1.0, 0.0, 4.0 };
  ofParameter<float> gammaParameter { "Gamma", 2.2, 0.1, 5.0 };
  ofParameter<float> whitePointParameter { "White Pt", 11.2, 1.0, 20.0 }; // for Reinhard Extended
  ofParameter<float> contrastParameter { "Contrast", 1.03, 0.9, 1.1 };
  ofParameter<float> saturationParameter { "Saturation", 1.0, 0.0, 2.0 };
  ofParameter<float> brightnessParameter { "Brightness", 0.0, -0.1, 0.1 };
  ofParameter<float> hueShiftParameter { "Hue Shift", 0.0, -1.0, 1.0 };
  ofParameter<float> sideExposureParameter { "Side Exp", 0.6, 0.0, 4.0 };
  ofxLabel recorderStatus;
  ofxLabel saveStatus;
  ofxLabel pauseStatus;
  
  bool guiVisible { true };

  bool plusKeyPressed { false };
  bool equalsKeyPressed { false };
  
#ifdef TARGET_MAC
  ofxFFmpegRecorder recorder;
  ofFbo recorderCompositeFbo;
#endif
  
  std::vector<SaveToFileThread*> saveToFileThreads;
  
  std::shared_ptr<LoggerChannel> loggerChannelPtr;
};



template <typename ModT>
ofxMarkSynth::ModPtr Synth::addMod(const std::string& name, ofxMarkSynth::ModConfig&& modConfig) {
  auto modPtr = std::make_shared<ModT>(this, name, std::forward<ofxMarkSynth::ModConfig>(modConfig));
  modPtrs.insert({ name, modPtr });
  return modPtr;
}

template <typename ModT, typename... Args>
ofxMarkSynth::ModPtr Synth::addMod(const std::string& name, ofxMarkSynth::ModConfig&& modConfig, Args&&... args) {
  auto modPtr = std::make_shared<ModT>(this, name, std::forward<ofxMarkSynth::ModConfig>(modConfig), std::forward<Args>(args)...);
  modPtrs.insert({ name, modPtr });
  return modPtr;
}


std::string saveFilePath(const std::string& filename);



} // ofxMarkSynth
