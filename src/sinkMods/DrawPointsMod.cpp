//
//  DrawPointsMod.cpp
//  example_points
//
//  Created by Steve Meyfroidt on 05/05/2025.
//

#include "DrawPointsMod.hpp"


namespace ofxMarkSynth {


DrawPointsMod::DrawPointsMod(const std::string& name, const ModConfig&& config, const glm::vec2 fboSize)
: Mod { name, std::move(config) }
{
  fbo.allocate(fboSize.x, fboSize.y, GL_RGBA32F); // 32F to accommodate fade, but this could be an optional thing to use a smaller FBO if no fade
}

void DrawPointsMod::initParameters() {
  parameters.add(pointRadiusParameter);
//  parameters.add(pointFadeParameter);
  parameters.add(colorParameter);
}

void DrawPointsMod::update() {
  fbo.begin();
  ofScale(fbo.getWidth(), fbo.getHeight());
  ofFill();
  ofSetColor(colorParameter);
  std::for_each(newPoints.begin(),
                newPoints.end(),
                [this](const auto& p) {
    ofDrawCircle(p, pointRadiusParameter / fbo.getWidth());
  });
  newPoints.clear();
  fbo.end();
}

void DrawPointsMod::draw() {
  fbo.draw(0.0, 0.0);
}

void DrawPointsMod::receive(int sinkId, const float& value) {
  switch (sinkId) {
    case SINK_POINT_RADIUS:
      pointRadiusParameter = value;
      break;
    default:
      ofLogError() << "float receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void DrawPointsMod::receive(int sinkId, const glm::vec2& point) {
  switch (sinkId) {
    case SINK_POINTS:
      newPoints.push_back(point);
      break;
    default:
      ofLogError() << "glm::vec2 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}


} // ofxMarkSynth
