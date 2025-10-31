//
//  PathMod.hpp
//  example_collage
//
//  Created by Steve Meyfroidt on 19/05/2025.
//

#pragma once

#include "Mod.hpp"
#include "ParamController.h"

namespace ofxMarkSynth {


class PathMod : public Mod {

public:
  PathMod(Synth* synthPtr, const std::string& name, const ModConfig&& config);
  void update() override;
  void draw() override;
  bool keyPressed(int key) override;
  void receive(int sinkId, const glm::vec2& v) override;
  void applyIntent(const Intent& intent, float strength) override;

  static constexpr int SINK_VEC2 = 1;
  static constexpr int SOURCE_PATH = 10;
  
protected:
  void initParameters() override;

private:
  float updateCount;
  ofParameter<int> strategyParameter { "Strategy", 0, 0, 3 }; // 0=polypath; 1=bounds; 2=horizontals; 3=convex hull
  ofParameter<int> maxVerticesParameter { "MaxVertices", 3, 0, 20 };
  ofParameter<float> maxVertexProximityParameter { "MaxVertexProximity", 0.07, 0.0, 1.0 };
  ParamController<float> maxVertexProximityController { maxVertexProximityParameter };
  ofParameter<float> minVertexProximityParameter { "MinVertexProximity", 0.01, 0.0, 1.0 };
  ParamController<float> minVertexProximityController { minVertexProximityParameter };
  
  std::deque<glm::vec2> newVecs;
  std::vector<glm::vec2> findCloseNewPoints() const;
  ofPath path;
  const ofPixels createPath();

  bool visible = false;
};


} // ofxMarkSynth
