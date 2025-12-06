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
#include "ParamController.h"


namespace ofxMarkSynth {


class DividedAreaMod : public Mod {

public:
  DividedAreaMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config);
  float getAgency() const override;
  void update() override;
  void receive(int sinkId, const glm::vec2& point) override;
  void receive(int sinkId, const ofPath& path) override;
  void receive(int sinkId, const float& point) override;
  void receive(int sinkId, const glm::vec4& v) override;
  void receive(int sinkId, const ofFbo& v) override;
  void applyIntent(const Intent& intent, float strength) override;

  static constexpr int SINK_MAJOR_ANCHORS = 1;
  static constexpr int SINK_MINOR_ANCHORS = 10;
  static constexpr int SINK_MINOR_PATH = 20;
  static constexpr int SINK_MINOR_LINES_COLOR = 30;
  static constexpr int SINK_MAJOR_LINES_COLOR = 31;
  static constexpr int SINK_BACKGROUND_SOURCE = 100; // for refraction on major lines
  static constexpr int SINK_CHANGE_ANGLE = 200;
  static constexpr int SINK_CHANGE_STRATEGY = 201;
  static constexpr int SINK_CHANGE_LAYER = 202;

  // DEFAULT_LAYERPTR_NAME is for drawing unconstrained lines
  static constexpr std::string MAJOR_LINES_LAYERPTR_NAME { "major-lines" };
  
protected:
  void initParameters() override;

private:
  ofParameter<int> strategyParameter { "Strategy", 0, 0, 2 }; // 0 = point pairs, 1 = point angles, 2 = radiating
  ofParameter<float> angleParameter { "Angle", 0.125, 0.0, 0.5 };
  ParamController<float> angleController { angleParameter };
  ofParameter<ofFloatColor> minorLineColorParameter { "MinorLineColour", ofFloatColor(0.0, 0.0, 0.0, 1.0), ofFloatColor(0.0, 0.0, 0.0, 0.0), ofFloatColor(1.0, 1.0, 1.0, 1.0) };
  ParamController<ofFloatColor> minorLineColorController { minorLineColorParameter };
  ofParameter<ofFloatColor> majorLineColorParameter { "MajorLineColour", ofFloatColor(0.0, 0.0, 0.0, 1.0), ofFloatColor(0.0, 0.0, 0.0, 0.0), ofFloatColor(1.0, 1.0, 1.0, 1.0) };
  ParamController<ofFloatColor> majorLineColorController { majorLineColorParameter };
  ofParameter<float> pathWidthParameter { "PathWidth", 0.0, 0.0, 0.01 };
  ParamController<float> pathWidthController { pathWidthParameter };
  ofParameter<float> majorLineWidthParameter { "MajorLineWidth", 200.0, 0.0, 500.0 };
  ParamController<float> majorLineWidthController { majorLineWidthParameter };
  ofParameter<float> maxUnconstrainedLinesParameter { "MaxUnconstrainedLines", 3.0, 1.0, 10.0 };
  ParamController<float> maxUnconstrainedLinesController { maxUnconstrainedLinesParameter };
  ofParameter<float> agencyFactorParameter { "AgencyFactor", 1.0, 0.0, 1.0 };
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
