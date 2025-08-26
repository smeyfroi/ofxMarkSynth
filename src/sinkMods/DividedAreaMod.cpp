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
  parameters.add(strategyParameter);
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

void DividedAreaMod::addConstrainedLinesThroughPointAngles() {
  float angle = angleParameter;
  std::for_each(newMinorAnchors.begin(), newMinorAnchors.end(), [this](const auto& p) {
    auto endPoint = endPointForSegment(p, angleParameter * glm::pi<float>(), 0.01);
    if (endPoint.x > 0.0 && endPoint.x < 1.0 && endPoint.y > 0.0 && endPoint.y < 1.0) {
      // must stay inside normalised coords
      dividedArea.addConstrainedDividerLine(p, endPoint);
    }
  });
  newMinorAnchors.clear();
}

void DividedAreaMod::addConstrainedLinesRadiating() {
  if (newMinorAnchors.size() < 7) return;
  glm::vec2 centrePoint = newMinorAnchors.back(); newMinorAnchors.pop_back();
  std::for_each(newMinorAnchors.begin(), newMinorAnchors.end(), [this, centrePoint](const auto& p) {
    dividedArea.addConstrainedDividerLine(centrePoint, p);
  });
  newMinorAnchors.clear();
}

void DividedAreaMod::update() {
  dividedArea.updateUnconstrainedDividerLines(newMajorAnchors); // assumes all the major anchors come at once (as the cluster centres)
  newMajorAnchors.clear();
  
  switch (strategyParameter) {
    case 0:
      addConstrainedLinesThroughPointPairs();
      break;
    case 1:
      addConstrainedLinesThroughPointAngles();
      break;
    case 2:
      addConstrainedLinesRadiating();
      break;
    default:
      break;
  }
  
  const float maxLineWidth = 160.0;
  const float minLineWidth = 110.0;

  // draw constrained
  auto fboPtr0 = fboPtrs[0];
  if (fboPtr0 != nullptr) {
    fboPtr0->getSource().begin();
    const ofFloatColor majorDividerColor { 0.0, 0.0, 0.0, 1.0 };
    dividedArea.draw({},
                     { minLineWidth, maxLineWidth, majorDividerColor },
                     {},
                     fboPtr0->getWidth());
//    ofSetColor(majorDividerColor);
//    dividedArea.draw(0.0, 0.0, 1.0, fboPtr0->getWidth());
    fboPtr0->getSource().end();
  }

  // draw unconstrained
  auto fboPtr1 = fboPtrs[1];
  if (fboPtr1 != nullptr) {
    fboPtr1->getSource().begin();
    const ofFloatColor minorDividerColor { 0.0, 0.0, 0.0, 1.0 };
    dividedArea.draw({},
                     {},
                     { minLineWidth*0.1f, minLineWidth*0.4f, minorDividerColor, 0.7 },
                     fboPtr0->getWidth());
//    ofSetColor(minorDividerColor);
//    dividedArea.draw(0.0, 20.0, 0.0, fboPtr1->getWidth());
    fboPtr1->getSource().end();
  }
}

void DividedAreaMod::receive(int sinkId, const glm::vec2& point) {
  switch (sinkId) {
    case SINK_MAJOR_ANCHORS:
      if (newMajorAnchors.size() < 1 || newMajorAnchors.back() != point) newMajorAnchors.push_back(point);
      break;
    case SINK_MINOR_ANCHORS:
      if (newMajorAnchors.size() < 1 || newMajorAnchors.back() != point) newMinorAnchors.push_back(point);
      break;
    default:
      ofLogError() << "glm::vec2 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}


} // ofxMarkSynth

