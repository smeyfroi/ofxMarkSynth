//
//  ClusterMod.hpp
//  example_audio_clusters
//
//  Created by Steve Meyfroidt on 07/05/2025.
//

#pragma once

#include "ofxGui.h"
#include "Mod.hpp"
#include "ofxPointClusters.h"
#include "Intent.hpp"
#include "ParamController.h"

namespace ofxMarkSynth {



class ClusterMod : public Mod {

public:
  ClusterMod(Synth* synthPtr, const std::string& name, const ModConfig&& config);
  float getAgency() const override;
  void update() override;
  void receive(int sinkId, const glm::vec2& v) override;
  void receive(int sinkId, const float& v) override;
  void applyIntent(const Intent& intent, float strength) override;

  static constexpr int SINK_VEC2 = 1;
  static constexpr int SINK_CHANGE_CLUSTER_NUM = 10;
  static constexpr int SOURCE_CLUSTER_CENTRE_VEC2 = 2;

protected:
  void initParameters() override;

private:
  std::unique_ptr<ParamController<float>> clustersControllerPtr;
  ofParameter<float> agencyFactorParameter { "Agency Factor", 1.0, 0.0, 1.0 }; // 0.0 -> No agency; 1.0 -> Global synth agency

  PointClusters pointClusters;

  std::vector<glm::vec2> newVecs;
};



} // ofxMarkSynth
