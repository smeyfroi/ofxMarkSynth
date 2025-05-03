//
//  PointIntrospectorMod.cpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#include "PointIntrospectorMod.hpp"


namespace ofxMarkSynth {


PointIntrospectorMod::PointIntrospectorMod(const std::string& name, const ModConfig&& config)
: Mod { name, std::move(config) }
{}

void PointIntrospectorMod::initParameters() {
  parameters.add(pointSizeParameter);
  parameters.add(pointFadeParameter);
  parameters.add(colorParameter);
}

void PointIntrospectorMod::update() {
  if (!introspectorPtr) { ofLogError() << "update in " << typeid(*this).name() << " with no introspector"; return; }
  std::for_each(newPoints.begin(), newPoints.end(), [&](const auto& p) {
    introspectorPtr->addCircle(p.x, p.y, pointSizeParameter/ofGetWindowWidth(), colorParameter, true, pointFadeParameter);
  });
  newPoints.clear();
}

void PointIntrospectorMod::receive(int sinkId, const glm::vec2& point) {
  switch (sinkId) {
    case SINK_POINTS:
      newPoints.push_back(point);
      break;
    default:
      ofLogError() << "receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}


} // ofxMarkSynth
