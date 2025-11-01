//
//  ClusterMod.cpp
//  example_audio_clusters
//
//  Created by Steve Meyfroidt on 07/05/2025.
//

#include "ClusterMod.hpp"
#include "IntentMapping.hpp"


namespace ofxMarkSynth {



int PointClustersAdaptor::getNumClusters() const {
  return static_cast<int>(ownerModPtr->clustersControllerPtr->value);
}



ClusterMod::ClusterMod(Synth* synthPtr, const std::string& name, const ModConfig&& config)
: Mod { synthPtr, name, std::move(config) }
{
  sinkNameIdMap = {
    { "vec2", SINK_VEC2 },
    { "changeClusterNum", SINK_CHANGE_CLUSTER_NUM }
  };
  sourceNameIdMap = {
    { "clusterCentreVec2", SOURCE_CLUSTER_CENTRE_VEC2 }
  };
}

void ClusterMod::initParameters() {
  parameters.add(pointClusters.getParameterGroup());
  parameters.add(agencyFactorParameter);
  clustersControllerPtr = std::make_unique<ParamController<float>>(pointClusters.clustersParameter);
}

float ClusterMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

void ClusterMod::update() {
  clustersControllerPtr->update();

  std::for_each(newVecs.cbegin(), newVecs.cend(), [this](const auto& v) {
    pointClusters.add(v);
  });
  newVecs.clear();

  auto clusters = pointClusters.getClusters(); // a copy from the cluster thread
  std::for_each(clusters.cbegin(), clusters.cend(), [this](const auto& v) {
    emit(SOURCE_CLUSTER_CENTRE_VEC2, v);
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
    case SINK_CHANGE_CLUSTER_NUM:
      {
        int newSize = pointClusters.getMinClusters() + v * static_cast<float>(pointClusters.getMaxClusters() - pointClusters.getMinClusters());
        ofLogNotice() << "ClusterMod::SINK_CHANGE_CLUSTER_NUM: changing size to " << newSize;
        clustersControllerPtr->updateAuto(static_cast<float>(newSize), getAgency());
      }
      break;
    default:
      ofLogError() << "float receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void ClusterMod::applyIntent(const Intent& intent, float strength) {
  if (strength < 0.01f) return;
  
  // Chaos -> number of clusters
  float targetClusters = linearMap(intent.getChaos(), *clustersControllerPtr);
  clustersControllerPtr->updateIntent(static_cast<float>(targetClusters), strength);
}


} // ofxMarkSynth
