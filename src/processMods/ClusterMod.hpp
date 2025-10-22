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


namespace ofxMarkSynth {


class ClusterMod : public Mod {

public:
  ClusterMod(const std::string& name, const ModConfig&& config);
  void update() override;
  void receive(int sinkId, const glm::vec2& v) override;
  void receive(int sinkId, const float& v) override;

  static constexpr int SINK_VEC2 = 1;
  static constexpr int SINK_CHANGE_CLUSTER_NUM = 10;
  static constexpr int SOURCE_VEC2 = 2;

protected:
  void initParameters() override;

private:
  PointClusters pointClusters;

  std::vector<glm::vec2> newVecs;
};


} // ofxMarkSynth
