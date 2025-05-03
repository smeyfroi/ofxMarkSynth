//
//  PointIntrospectorMod.hpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#pragma once

#include "ofxGui.h"
#include "Mod.hpp"
#include "ofxIntrospector.h"


namespace ofxMarkSynth {


class IntrospectorMod : public Mod {

public:
  IntrospectorMod(const std::string& name, const ModConfig&& config);
  void update() override;
  void receive(int sinkId, const float& point) override;
  void receive(int sinkId, const glm::vec2& point) override;
  std::shared_ptr<Introspector> introspectorPtr;

  static constexpr int SINK_POINTS = 1;
  static constexpr int SINK_HORIZONTAL_LINES_1 = 10;
  static constexpr int SINK_HORIZONTAL_LINES_2 = 11;
  static constexpr int SINK_HORIZONTAL_LINES_3 = 12;

protected:
  void initParameters() override;

private:
  ofParameter<float> pointSizeParameter { "PointSize", 1.0, 0.0, 4.0 };
  ofParameter<int> pointFadeParameter { "PointFade", 30, 0, 240 };
  ofParameter<ofColor> colorParameter { "Color", ofColor::yellow, ofColor(0, 255), ofColor(255, 255) };
  ofParameter<int> horizontalLineFadeParameter { "HorizontalLineFade", 30, 0, 240 };
  ofParameter<ofColor> horizontalLine1ColorParameter { "HorizontalLine1Color", ofColor::darkBlue, ofColor(0, 255), ofColor(255, 255) };
  ofParameter<ofColor> horizontalLine2ColorParameter { "HorizontalLine2Color", ofColor::darkGray, ofColor(0, 255), ofColor(255, 255) };
  ofParameter<ofColor> horizontalLine3ColorParameter { "HorizontalLine3Color", ofColor::darkGreen, ofColor(0, 255), ofColor(255, 255) };

  std::vector<glm::vec2> newPoints;
  std::vector<float> newHorizontalLines1;
  std::vector<float> newHorizontalLines2;
  std::vector<float> newHorizontalLines3;
};


} // ofxMarkSynth
