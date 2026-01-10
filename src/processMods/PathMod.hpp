//
//  PathMod.hpp
//  example_collage
//
//  Created by Steve Meyfroidt on 19/05/2025.
//

#pragma once

#include <deque>
#include <vector>
#include "core/Mod.hpp"
#include "core/ParamController.h"

namespace ofxMarkSynth {


class PathMod : public Mod {

public:
  PathMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config, bool triggerBased = false);
  float getAgency() const override;
  void update() override;
  void draw() override;
  bool keyPressed(int key) override;
  void receive(int sinkId, const glm::vec2& v) override;
  void receive(int sinkId, const float& v) override;
  void applyIntent(const Intent& intent, float strength) override;

  UiState captureUiState() const override {
    UiState state;
    setUiStateBool(state, "visible", visible);
    return state;
  }

  void restoreUiState(const UiState& state) override {
    visible = getUiStateBool(state, "visible", visible);
  }

  static constexpr int SINK_VEC2 = 1;
  static constexpr int SINK_TRIGGER = 2;
  static constexpr int SOURCE_PATH = 10;
  
protected:
  void initParameters() override;

private:
  bool triggerBased { false };  // Set via constructor from ModFactory
  
  ofParameter<int> strategyParameter { "Strategy", 0, 0, 3 }; // 0=polypath; 1=bounds; 2=horizontals; 3=convex hull
  ofParameter<float> maxVerticesParameter { "MaxVertices", 3, 0, 20 };
  ParamController<float> maxVerticesController { maxVerticesParameter };
  ofParameter<float> clusterRadiusParameter { "ClusterRadius", 0.15, 0.01, 1.0 };
  ParamController<float> clusterRadiusController { clusterRadiusParameter };
  ofParameter<float> agencyFactorParameter { "AgencyFactor", 1.0, 0.0, 1.0 };
  
  std::deque<glm::vec2> newVecs;
  std::vector<glm::vec2> findCloseNewPoints() const;
  void emitPathFromClusteredPoints();  // Emit path and retain unused points
  ofPath path;
  const ofPixels createPath();

  bool visible = false;
};


} // ofxMarkSynth
