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
  
  if (!newMinorAnchors.empty()) {
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
  }
  
  const float maxLineWidth = 160.0;
  const float minLineWidth = 110.0;

  // draw unconstrained
  auto fboPtr0 = fboPtrs[0];
  if (fboPtr0 != nullptr) {
    fboPtr0->getSource().begin();
    const ofFloatColor majorDividerColor { 0.0, 0.0, 0.0, 1.0 };
    dividedArea.draw({},
                     { minLineWidth, maxLineWidth, majorDividerColor },
                     {},
                     fboPtr0->getWidth());
//    ofSetColor(majorDividerColor);
//    dividedArea.draw(0.0, 20.0, 0.0, fboPtr1->getWidth());
    fboPtr0->getSource().end();
  }

  // draw constrained
  auto fboPtr1 = fboPtrs[1];
  if (fboPtr1 != nullptr) {
    fboPtr1->getSource().begin();
    const ofFloatColor minorDividerColor { 0.0, 0.0, 0.0, 1.0 };
    dividedArea.draw({},
                     {},
                     { minLineWidth*0.15f, minLineWidth*0.25f, minorDividerColor, 0.7 },
                     fboPtr0->getWidth());
//    ofSetColor(minorDividerColor);
//    dividedArea.draw(0.0, 0.0, 10.0, fboPtr0->getWidth());
    fboPtr1->getSource().end();
  }
}

void DividedAreaMod::receive(int sinkId, const glm::vec2& point) {
  switch (sinkId) {
    case SINK_MAJOR_ANCHORS:
      if (newMajorAnchors.empty() || newMajorAnchors.back() != point) newMajorAnchors.push_back(point);
      break;
    case SINK_MINOR_ANCHORS:
      if (newMinorAnchors.empty() || newMinorAnchors.back() != point) newMinorAnchors.push_back(point);
      break;
    default:
      ofLogError() << "glm::vec2 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void DividedAreaMod::receive(int sinkId, const ofPath& path) {
  switch (sinkId) {
    case SINK_MINOR_PATH:
      // fetch all the points of the path
      for (auto& poly : path.getOutline()) {
        auto vertices = poly.getVertices();
        glm::vec2 previousVertex { vertices.back() };
        for (auto& v : vertices) {
          glm::vec2 p { v };
          if (newMinorAnchors.empty() || newMinorAnchors.back() != p) {
            newMinorAnchors.push_back(previousVertex);
            newMinorAnchors.push_back(p);
            previousVertex = p;
          }
        }
      }
      break;
    default:
      ofLogError() << "ofPath receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void DividedAreaMod::receive(int sinkId, const float& v) {
  switch (sinkId) {
    case SINK_AUDIO_ONSET:
      {
        int newStrategy = (strategyParameter + 1) % 3;
        ofLogNotice() << "DividedAreaMod::receive audio onset; changing strategy to " << newStrategy;
        strategyParameter = newStrategy;
        strategyChangeInvalidUntilTimestamp = ofGetElapsedTimef() + 5.0; // 5s
      }
      break;
    case SINK_AUDIO_TIMBRE_CHANGE:
      {
        float newAngle = ofRandom(0.0, 0.5);
        ofLogNotice() << "DividedAreaMod::receive audio timbre change; changing angle to " << newAngle;
        angleParameter = newAngle;
      }
      break;
    default:
      ofLogError() << "float receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

float DividedAreaMod::bidToReceive(int sinkId) {
  switch (sinkId) {
    case SINK_AUDIO_ONSET:
      if (ofGetElapsedTimef() < strategyChangeInvalidUntilTimestamp) return 0.0;
      return 0.3;
    case SINK_AUDIO_TIMBRE_CHANGE:
      if (strategyParameter == 1) return 0.4; // angle strategy
    default:
      return 0.0;
  }
}



} // ofxMarkSynth

