//
//  ClusterMod.cpp
//  example_audio_clusters
//
//  Created by Steve Meyfroidt on 07/05/2025.
//

#include "ClusterMod.hpp"


namespace ofxMarkSynth {


ClusterMod::ClusterMod(const std::string& name, const ModConfig&& config)
: Mod { name, std::move(config) }
{}

void ClusterMod::initParameters() {
  parameters.add(pointClusters.getParameterGroup());
}

void ClusterMod::update() {
  std::for_each(newVecs.cbegin(), newVecs.cend(), [this](const auto& v) {
    pointClusters.add(v);
  });
  newVecs.clear();

  auto clusters = pointClusters.getClusters(); // a copy from the cluster thread
  std::for_each(clusters.cbegin(), clusters.cend(), [this](const auto& v) {
    emit(SOURCE_VEC2, v);
  });
  
  pointClusters.update();
}

void ClusterMod::receive(int sinkId, const glm::vec2& v) {
  switch (sinkId) {
    case SINK_VEC2:
      newVecs.push_back(v);
      break;
    default:
      ofLogError() << "glm::vec2 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}


} // ofxMarkSynth
