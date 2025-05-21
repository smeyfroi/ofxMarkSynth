//
//  PathMod.hpp
//  example_collage
//
//  Created by Steve Meyfroidt on 19/05/2025.
//

#pragma once

#include "Mod.hpp"

namespace ofxMarkSynth {


class PathMod : public Mod {

public:
  PathMod(const std::string& name, const ModConfig&& config);
  void update() override;
  void draw() override;
  bool keyPressed(int key) override;
  void receive(int sinkId, const glm::vec2& v) override;

  static constexpr int SINK_VEC2 = 1;
  static constexpr int SOURCE_PATH = 10;

protected:
  void initParameters() override;

private:
  float updateCount;
  ofParameter<int> maxVerticesParameter { "MaxVertices", 3, 0, 20 };
  ofParameter<float> vertexProximityParameter { "VertexProximity", 0.1, 0.0, 1.0 };
  
  std::deque<glm::vec2> newVecs;
  ofPath path;
  const ofPixels createPath();

  bool visible = false;
};


} // ofxMarkSynth
