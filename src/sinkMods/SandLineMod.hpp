//
//  SandLineMod.hpp
//  example_sandlines
//
//  Created by Steve Meyfroidt on 10/05/2025.
//

#pragma once

#include <vector>
#include "ofxGui.h"
#include "core/Mod.hpp"
#include "PingPongFbo.h"
#include "core/ParamController.h"



namespace ofxMarkSynth {



class SandLineMod : public Mod {

public:
  SandLineMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config);
  float getAgency() const override;
  void update() override;
  void receive(int sinkId, const float& value) override;
  void receive(int sinkId, const glm::vec2& point) override;
  void receive(int sinkId, const glm::vec4& v) override;
  void applyIntent(const Intent& intent, float strength) override;

  static constexpr int SINK_POINTS = 1;
  static constexpr int SINK_POINT_RADIUS = 10;
  static constexpr int SINK_POINT_COLOR = 20;

protected:
  void initParameters() override;

private:
  void drawSandLine(glm::vec2 p1, glm::vec2 p2, float drawScale);

  ofParameter<float> densityParameter { "Density", 0.2, 0.05, 0.5 };
  ParamController<float> densityController { densityParameter };
  ofParameter<float> pointRadiusParameter { "PointRadius", 1.0, 0.0, 32.0 };
  ParamController<float> pointRadiusController { pointRadiusParameter };
  ofParameter<ofFloatColor> colorParameter { "Colour", ofFloatColor { 1.0, 1.0, 1.0, 1.0 }, ofFloatColor { 0.0, 0.0, 0.0, 0.0 }, ofFloatColor { 1.0, 1.0, 1.0, 1.0 } };
  ParamController<ofFloatColor> colorController { colorParameter };
  ofParameter<float> alphaMultiplierParameter { "AlphaMultiplier", 0.05, 0.0, 1.0 };
  ParamController<float> alphaMultiplierController { alphaMultiplierParameter };
  ofParameter<float> stdDevAlongParameter { "StdDevAlong", 0.5, 0.0, 1.0 };
  ParamController<float> stdDevAlongController { stdDevAlongParameter };
  ofParameter<float> stdDevPerpendicularParameter { "StdDevPerpendicular", 0.005, 0.0, 0.02 };
  ParamController<float> stdDevPerpendicularController { stdDevPerpendicularParameter };
  ofParameter<float> agencyFactorParameter { "AgencyFactor", 1.0, 0.0, 1.0 };

  std::vector<glm::vec2> newPoints;
};



} // ofxMarkSynth
