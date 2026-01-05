//
//  NodeEditorModel.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 07/11/2025.
//

#include "NodeEditorModel.hpp"
#include "NodeEditorLayoutSerializer.hpp"
#include "../core/Synth.hpp"
#include "imnodes.h"
#include <algorithm>



namespace ofxMarkSynth {



void NodeEditorModel::buildFromSynth(const std::shared_ptr<Synth> synthPtr_) {
  synthPtr = synthPtr_;
  nodes.clear();
  layoutEnginePtr = std::make_unique<NodeEditorLayout>();
  
  // Build node list from Synth and its modPtrs
  nodes.emplace_back(NodeEditorNode {
    .objectPtr = synthPtr,
    .position = glm::vec2(50.0f, 100.0f),
  });
  std::vector<std::string> modNames;
  modNames.reserve(synthPtr->modPtrs.size());
  for (const auto& [modName, modPtr] : synthPtr->modPtrs) {
    modNames.push_back(modName);
  }
  std::sort(modNames.begin(), modNames.end());

  for (const auto& modName : modNames) {
    auto modPtr = synthPtr->modPtrs.at(modName);
    nodes.emplace_back(NodeEditorNode {
      .objectPtr = modPtr,
      .position = glm::vec2(100.0f, 100.0f),
    });
    layoutEnginePtr->addNode(modPtr);
  }

  std::vector<std::string> layerNames;
  layerNames.reserve(synthPtr->getDrawingLayers().size());
  for (const auto& [layerName, layerPtr] : synthPtr->getDrawingLayers()) {
    layerNames.push_back(layerName);
  }
  std::sort(layerNames.begin(), layerNames.end());

  for (const auto& layerName : layerNames) {
    auto layerPtr = synthPtr->getDrawingLayers().at(layerName);
    nodes.emplace_back(NodeEditorNode {
      .objectPtr = layerPtr,
      .position = glm::vec2(100.0f, 150.0f),
    });
    layoutEnginePtr->addNode(layerPtr);
  }

  // Establish a deterministic initial layout (and any per-synth precomputation).
  layoutEnginePtr->initialize(synthPtr);
}

int getId(NodeObjectPtr objectPtr) {
  if (const auto objectPtrPtr = std::get_if<std::shared_ptr<Mod>>(&objectPtr)) {
    return (*objectPtrPtr)->getId();
  } else if (const auto objectPtrPtr = std::get_if<std::shared_ptr<DrawingLayer>>(&objectPtr)) {
    return objectPtrPtr->get()->id;
  } else {
    ofLogError("NodeEditorModel") << "getId called with unknown NodeObjectPtr type";
    return 0;
  }
}

void NodeEditorModel::computeLayout() {
  if (!layoutEnginePtr) return;
  for (auto& node : nodes) {
    glm::vec2 pos = layoutEnginePtr->getNodePosition(node.objectPtr);
    node.position = pos;
    ImNodes::SetNodeGridSpacePos(getId(node.objectPtr), ImVec2(pos.x, pos.y));
  }
}

void NodeEditorModel::computeLayoutAnimated() {
  if (!layoutEnginePtr) return;
  if (layoutEnginePtr->step()) computeLayout();
}

bool NodeEditorModel::isLayoutAnimating() const {
  return layoutEnginePtr && !layoutEnginePtr->isStable();
}

void NodeEditorModel::resetLayout() {
  buildFromSynth(synthPtr);
}

void NodeEditorModel::randomizeLayout() {
  if (!layoutEnginePtr) return;
  layoutEnginePtr->randomize();
  computeLayout();
}

void NodeEditorModel::relaxLayout(int iterations) {
  if (!layoutEnginePtr) return;
  layoutEnginePtr->compute(iterations);
  computeLayout();
}

void NodeEditorModel::syncPositionsFromImNodes() {
  for (auto& node : nodes) {
    ImVec2 imnodesPos = ImNodes::GetNodeGridSpacePos(getId(node.objectPtr));
    node.position = glm::vec2(imnodesPos.x, imnodesPos.y);
  }
}

bool NodeEditorModel::saveLayout() {
  if (!layoutEnginePtr) return false;
  bool success = NodeEditorLayoutSerializer::save(*this, synthPtr->name, synthPtr->getCurrentConfigPath());
  if (success) {
    snapshotPositions();  // Update snapshot after successful save
  }
  return success;
}

bool NodeEditorModel::loadLayout() {
  if (!layoutEnginePtr) return false;
  bool success = NodeEditorLayoutSerializer::load(*this, synthPtr->name, synthPtr->getCurrentConfigPath());
  if (success) {
    snapshotPositions();  // Update snapshot after successful load
  }
  return success;
}

bool NodeEditorModel::hasStoredLayout() const {
  if (!layoutEnginePtr) return false;
  return NodeEditorLayoutSerializer::exists(synthPtr->name, synthPtr->getCurrentConfigPath());
}

bool NodeEditorModel::hasPositionsChanged() const {
  if (lastKnownPositions.size() != nodes.size()) return true;
  
  constexpr float EPSILON = 0.5f;  // Small threshold to ignore sub-pixel drift
  for (size_t i = 0; i < nodes.size(); ++i) {
    if (glm::distance(nodes[i].position, lastKnownPositions[i]) > EPSILON) {
      return true;
    }
  }
  return false;
}

void NodeEditorModel::snapshotPositions() {
  lastKnownPositions.clear();
  lastKnownPositions.reserve(nodes.size());
  for (const auto& node : nodes) {
    lastKnownPositions.push_back(node.position);
  }
}



} // namespace ofxMarkSynth
