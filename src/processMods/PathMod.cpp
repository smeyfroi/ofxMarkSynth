//
//  PathMod.cpp
//  example_collage
//
//  Created by Steve Meyfroidt on 19/05/2025.
//

#include "PathMod.hpp"


namespace ofxMarkSynth {


PathMod::PathMod(const std::string& name, const ModConfig&& config)
: Mod { name, std::move(config) }
{}

void PathMod::initParameters() {
  parameters.add(maxVerticesParameter);
  parameters.add(vertexProximityParameter);
}

void PathMod::update() {
  ofPath newPath;
  glm::vec2 previousVec { std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
  std::for_each(newVecs.crbegin(), newVecs.crend(), [&](const auto& v) { // start from the back, which is the newest points
    if (previousVec.x != std::numeric_limits<float>::max() && glm::distance2(previousVec, v) > vertexProximityParameter) return;
    newPath.lineTo(v);
    previousVec = v;
  });
  
  if (newPath.getCommands().size() < 4) {
    if (newVecs.size() > maxVerticesParameter * 3.0) newVecs.pop_front(); // not finding anything so pop one to avoid growing too large
    return;
  }
  
  newVecs.clear();
  path = newPath;
  path.close();
  path.setColor(ofFloatColor(1.0, 1.0, 1.0, 1.0));
  path.setFilled(true);
  newVecs.clear();
  emit(SOURCE_PATH, path);
}

void PathMod::draw() {
  if (!visible) return;
  ofPushMatrix();
  {
    ofScale(ofGetWindowWidth(), ofGetWindowHeight());
    path.draw();
  }
  ofPopMatrix();
}

bool PathMod::keyPressed(int key) {
  if (key == 'A') {
    visible = not visible;
    return true;
  }
  return false;
}

void PathMod::receive(int sinkId, const glm::vec2& v) {
  switch (sinkId) {
    case SINK_VEC2:
      if (!newVecs.empty() && newVecs.back() == v) return;
      newVecs.push_back(v);
      break;
    default:
      ofLogError() << "glm::vec2 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}


} // ofxMarkSynth
