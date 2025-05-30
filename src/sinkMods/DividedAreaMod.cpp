//
//  DividedAreaMod.cpp
//  example_audio_clusters
//
//  Created by Steve Meyfroidt on 07/05/2025.
//

#include "DividedAreaMod.hpp"
#include "LineGeom.h"


namespace ofxMarkSynth {


DividedAreaMod::DividedAreaMod(const std::string& name, const ModConfig&& config)
: Mod { name, std::move(config) },
dividedArea({ { 1.0, 1.0 }, 7 }) // normalised area size
{}

void DividedAreaMod::initParameters() {
  parameters.add(angleParameter);
  parameters.add(dividedArea.getParameterGroup());
}

void DividedAreaMod::addConstrainedLinesThroughPointPairs() {
  int pairs = newMinorAnchors.size() / 2;
  for (int i = 0; i < pairs; i++) {
    auto p1 = newMinorAnchors.back();
    newMinorAnchors.pop_back();
    auto p2 = newMinorAnchors.back();
    newMinorAnchors.pop_back();
    dividedArea.addConstrainedDividerLine(p1, p2);
  }
}

// glm::vec2 endPointForSegment(const glm::vec2& startPoint, float angleRadians, float length) {
void DividedAreaMod::addConstrainedLinesThroughPointAngles() {
  float angle = angleParameter;
  std::for_each(newMinorAnchors.begin(), newMinorAnchors.end(), [this](const auto& p) {
    auto endPoint = endPointForSegment(p, angleParameter * glm::pi<float>(), 0.01);
    dividedArea.addConstrainedDividerLine(p, endPoint); // must stay inside normalised coords
  });
}

void DividedAreaMod::update() {
  dividedArea.updateUnconstrainedDividerLines(newMajorAnchors); // assumes all the major anchors come at once (as the cluster centres)
  newMajorAnchors.clear();
  
  addConstrainedLinesThroughPointPairs();
//  addConstrainedLinesThroughPointAngles();
  
  // draw constrained
  auto fboPtr0 = fboPtrs[0];
  if (fboPtr0 != nullptr) {
    fboPtr0->getSource().begin();
    ofSetColor(ofFloatColor(0.0, 0.0, 0.0, 1.0));
    dividedArea.draw(0.0, 0.0, 1.0, fboPtr0->getWidth());
    fboPtr0->getSource().end();
  }

  // draw unconstrained
  auto fboPtr1 = fboPtrs[1];
  if (fboPtr1 != nullptr) {
    fboPtr1->getSource().begin();
    ofSetColor(ofFloatColor(0.0, 0.0, 0.0, 1.0));
    dividedArea.draw(0.0, 20.0, 0.0, fboPtr1->getWidth());
    fboPtr1->getSource().end();
  }
}

void DividedAreaMod::receive(int sinkId, const glm::vec2& point) {
  switch (sinkId) {
    case SINK_MAJOR_ANCHORS:
      newMajorAnchors.push_back(point);
      break;
    case SINK_MINOR_ANCHORS:
      newMinorAnchors.push_back(point);
      break;
    default:
      ofLogError() << "glm::vec2 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}


} // ofxMarkSynth

