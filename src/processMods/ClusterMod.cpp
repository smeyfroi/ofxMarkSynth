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

void ClusterMod::receive(int sinkId, const float& v) {
  switch (sinkId) {
    case SINK_AUDIO_ONSET:
      {
        int newSize = pointClusters.getMinClusters() + ofRandom(pointClusters.getMaxClusters() - pointClusters.getMinClusters());
        ofLogNotice() << "ClusterMod::receive audio onset; changing size to " << newSize;
        pointClusters.clustersParameter.set(newSize);
      }
      break;
    default:
      ofLogError() << "float receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

float ClusterMod::bidToReceive(int sinkId) {
  switch (sinkId) {
    case SINK_AUDIO_ONSET:
      if (pointClusters.size() >= pointClusters.getMinClusters() && pointClusters.size() <= pointClusters.getMaxClusters()) return 0.1;
    default:
      return 0.0;
  }
}


} // ofxMarkSynth
