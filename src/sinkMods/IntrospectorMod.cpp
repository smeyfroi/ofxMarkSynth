//
//  IntrospectorMod.cpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#include "IntrospectorMod.hpp"


namespace ofxMarkSynth {


IntrospectorMod::IntrospectorMod(Synth* synthPtr, const std::string& name, const ModConfig&& config)
: Mod { synthPtr, name, std::move(config) }
{
  introspectorPtr = std::make_shared<Introspector>();
  introspectorPtr->visible = true;

  sinkNameIdMap = {
    { "points", SINK_POINTS },
    { "horizontalLines1", SINK_HORIZONTAL_LINES_1 },
    { "horizontalLines2", SINK_HORIZONTAL_LINES_2 },
    { "horizontalLines3", SINK_HORIZONTAL_LINES_3 }
  };
}

void IntrospectorMod::initParameters() {
  parameters.add(pointSizeParameter);
  parameters.add(pointFadeParameter);
  parameters.add(colorParameter);
  parameters.add(horizontalLineFadeParameter);
  parameters.add(horizontalLine1ColorParameter);
  parameters.add(horizontalLine2ColorParameter);
  parameters.add(horizontalLine3ColorParameter);
}

void IntrospectorMod::update() {
  if (!introspectorPtr) { ofLogError() << "update in " << typeid(*this).name() << " with no introspector"; return; }
  
  introspectorPtr->update();

  std::for_each(newPoints.begin(), newPoints.end(), [&](const auto& p) {
    introspectorPtr->addCircle(p.x, p.y, pointSizeParameter/ofGetWindowWidth(), colorParameter, true, pointFadeParameter);
  });
  newPoints.clear();

  std::for_each(newHorizontalLines1.begin(), newHorizontalLines1.end(), [&](const auto& v) {
    introspectorPtr->addLine(0, v, 1.0, v, horizontalLine1ColorParameter, horizontalLineFadeParameter);
  });
  newHorizontalLines1.clear();

  std::for_each(newHorizontalLines2.begin(), newHorizontalLines2.end(), [&](const auto& v) {
    introspectorPtr->addLine(0, v, 1.0, v, horizontalLine2ColorParameter, horizontalLineFadeParameter);
  });
  newHorizontalLines2.clear();

  std::for_each(newHorizontalLines3.begin(), newHorizontalLines3.end(), [&](const auto& v) {
    introspectorPtr->addLine(0, v, 1.0, v, horizontalLine3ColorParameter, horizontalLineFadeParameter);
  });
  newHorizontalLines3.clear();
}

void IntrospectorMod::draw() {
  introspectorPtr->draw(1.0);
}

bool IntrospectorMod::keyPressed(int key) {
  return introspectorPtr->keyPressed(key);
}

void IntrospectorMod::receive(int sinkId, const float& value) {
  switch (sinkId) {
    case SINK_HORIZONTAL_LINES_1:
      newHorizontalLines1.push_back(value);
      break;
    case SINK_HORIZONTAL_LINES_2:
      newHorizontalLines2.push_back(value);
      break;
    case SINK_HORIZONTAL_LINES_3:
      newHorizontalLines3.push_back(value);
      break;
    default:
      ofLogError() << "float receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void IntrospectorMod::receive(int sinkId, const glm::vec2& point) {
  switch (sinkId) {
    case SINK_POINTS:
      newPoints.push_back(point);
      break;
    default:
      ofLogError() << "glm::vec2 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}


} // ofxMarkSynth
