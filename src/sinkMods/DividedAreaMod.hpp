//
//  DividedAreaMod.hpp
//  example_audio_clusters
//
//  Created by Steve Meyfroidt on 07/05/2025.
//

#pragma once

#include "ofxGui.h"
#include "Mod.hpp"
#include "ofxDividedArea.h"


namespace ofxMarkSynth {


// SINK_FBO is used to draw unconstrained lines
// SINK_FBO_1 is used to draw constrained lines
class DividedAreaMod : public Mod {

public:
  DividedAreaMod(const std::string& name, const ModConfig&& config);
  void update() override;
  void receive(int sinkId, const glm::vec2& point) override;
  void receive(int sinkId, const ofPath& path) override;
  void receive(int sinkId, const float& point) override;
  void receive(int sinkId, const glm::vec4& v) override;
  void receive(int sinkId, const ofFbo& v) override;
  float bidToReceive(int sinkId) override;

  static constexpr int SINK_MAJOR_ANCHORS = 1;
  static constexpr int SINK_MINOR_ANCHORS = 10;
  static constexpr int SINK_MINOR_PATH = 20;
  static constexpr int SINK_MINOR_LINES_COLOR = 30;
  static constexpr int SINK_MAJOR_LINES_COLOR = 31;
  static constexpr int SINK_BACKGROUND_SOURCE = 100; // for refraction on major lines

  // DEFAULT_FBOPTR_NAME is for drawing unconstrained lines
  static constexpr std::string MAJOR_LINES_FBOPTR_NAME { "major-lines" };
  
protected:
  void initParameters() override;

private:
  ofParameter<int> strategyParameter { "Strategy", 0, 0, 2 }; // 0 = point pairs, 1 = point angles, 2 = radiating
  ofParameter<float> angleParameter { "Angle", 0.125, 0.0, 0.5 };
  ofParameter<ofFloatColor> minorLineColorParameter { "MinorLineColor", ofFloatColor(0.0, 0.0, 0.0, 1.0), ofFloatColor(0.0, 0.0, 0.0, 0.0), ofFloatColor(1.0, 1.0, 1.0, 1.0) };
  ofParameter<ofFloatColor> majorLineColorParameter { "MajorLineColor", ofFloatColor(0.0, 0.0, 0.0, 1.0), ofFloatColor(0.0, 0.0, 0.0, 0.0), ofFloatColor(1.0, 1.0, 1.0, 1.0) };
  ofParameter<float> pathWidthParameter { "PathWidth", 0.0, 0.0, 0.01 };
  float strategyChangeInvalidUntilTimestamp = 0.0;

  std::vector<glm::vec2> newMajorAnchors;
  std::vector<glm::vec2> newMinorAnchors;
  DividedArea dividedArea;
  
  void addConstrainedLinesThroughPointPairs(float width = -1.0); // default delegates to DividedArea
  void addConstrainedLinesThroughPointAngles();
  void addConstrainedLinesRadiating();

  ofFbo backgroundFbo;
};


} // ofxMarkSynth
