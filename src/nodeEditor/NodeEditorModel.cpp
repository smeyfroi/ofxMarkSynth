//
//  NodeEditorModel.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 07/11/2025.
//

#include "nodeEditor/NodeEditorModel.hpp"
#include "nodeEditor/NodeEditorLayoutSerializer.hpp"
#include "core/Synth.hpp"
#include "imnodes.h"



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
  for (const auto& modNamePtr : synthPtr->modPtrs) {
    auto& [modName, modPtr] = modNamePtr;
    nodes.emplace_back(NodeEditorNode {
      .objectPtr = modPtr,
      .position = glm::vec2(100.0f, 100.0f),
    });
    layoutEnginePtr->addNode(modPtr);
  }
  for (const auto& layerNamePtr : synthPtr->getDrawingLayers()) {
    auto& [layerName, layerPtr] = layerNamePtr;
    nodes.emplace_back(NodeEditorNode {
      .objectPtr = layerPtr,
      .position = glm::vec2(100.0f, 150.0f),
    });
    layoutEnginePtr->addNode(layerPtr);
  }
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

void NodeEditorModel::syncPositionsFromImNodes() {
  for (auto& node : nodes) {
    ImVec2 imnodesPos = ImNodes::GetNodeGridSpacePos(getId(node.objectPtr));
    node.position = glm::vec2(imnodesPos.x, imnodesPos.y);
  }
}

bool NodeEditorModel::saveLayout() const {
  if (!layoutEnginePtr) return false;
  return NodeEditorLayoutSerializer::save(*this, synthPtr->name);
}

bool NodeEditorModel::loadLayout() {
  if (!layoutEnginePtr) return false;
  return NodeEditorLayoutSerializer::load(*this, synthPtr->name);
}

bool NodeEditorModel::hasStoredLayout() const {
  if (!layoutEnginePtr) return false;
  return NodeEditorLayoutSerializer::exists(synthPtr->name);
}



} // namespace ofxMarkSynth
