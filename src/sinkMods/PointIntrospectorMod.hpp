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


class PointIntrospectorMod : public Mod {

public:
  PointIntrospectorMod(const std::string& name, const ModConfig&& config);
  void update() override;
  void receive(int sinkId, const glm::vec2& point) override;
  std::shared_ptr<Introspector> introspectorPtr;

  static const int SINK_POINTS = 1;

protected:
  void initParameters() override;

private:
  ofParameter<float> pointSizeParameter { "PointSize", 1.0, 0.0, 4.0 };
  ofParameter<int> pointFadeParameter { "PointFade", 30, 0, 240 };
  ofParameter<ofColor> colorParameter { "Color", ofColor::yellow, ofColor(0, 255), ofColor(255, 255) };
  
  std::vector<glm::vec2> newPoints;
};


} // ofxMarkSynth
