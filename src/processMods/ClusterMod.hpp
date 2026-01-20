//  ClusterMod.hpp
//  example_audio_clusters
//
//  Created by Steve Meyfroidt on 07/05/2025.
//

#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "ofxGui.h"
#include "core/Mod.hpp"
#include "ofxPointClusters.h"
#include "core/Intent.hpp"
#include "core/ParamController.h"

namespace ofxMarkSynth {



class ClusterMod : public Mod {

public:
  ClusterMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config);
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
  ofParameter<float> agencyFactorParameter { "AgencyFactor", 1.0, 0.0, 1.0 }; // 0.0 -> No agency; 1.0 -> Global synth agency

  std::optional<int> lastAppliedNumClustersOverride;

  PointClusters pointClusters;

  std::vector<glm::vec2> newVecs;
};



} // ofxMarkSynth
