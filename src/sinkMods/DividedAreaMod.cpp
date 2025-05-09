//
//  DividedAreaMod.cpp
//  example_audio_clusters
//
//  Created by Steve Meyfroidt on 07/05/2025.
//

#include "DividedAreaMod.hpp"


namespace ofxMarkSynth {


DividedAreaMod::DividedAreaMod(const std::string& name, const ModConfig&& config)
: Mod { name, std::move(config) },
dividedArea({ { 1.0, 1.0 }, 7 }) // normalised area size
{}

void DividedAreaMod::initParameters() {
  parameters.add(dividedArea.getParameterGroup());
}

void DividedAreaMod::update() {
  dividedArea.updateUnconstrainedDividerLines(newMajorAnchors); // assumes all the major anchors come at once (as the cluster centres)
  newMajorAnchors.clear();
  
  int pairs = newMinorAnchors.size() / 2;
  for (int i = 0; i < pairs; i++) {
    auto p1 = newMinorAnchors.back();
    newMinorAnchors.pop_back();
    auto p2 = newMinorAnchors.back();
    newMinorAnchors.pop_back();
    dividedArea.addConstrainedDividerLine(p1, p2);
  }
}

void DividedAreaMod::draw() {
  dividedArea.draw(1.0, 1.0, 1.0, ofGetWindowWidth());
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

