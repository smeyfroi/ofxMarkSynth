//
//  NodeEditorModel.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 07/11/2025.
//

#include "NodeEditorModel.hpp"
#include "NodeEditorLayoutSerializer.hpp"
#include "Synth.hpp"
#include "imnodes.h"



namespace ofxMarkSynth {



void NodeEditorModel::buildFromSynth(const std::shared_ptr<Synth> synthPtr_) {
  synthPtr = synthPtr_;
  nodes.clear();
  layoutInitialized = false;
  
  // Build node list from Synth and its modPtrs
  nodes.emplace_back(NodeEditorNode {
    .modPtr = synthPtr,
    .position = glm::vec2(50.0f, 100.0f),
  });
  for (const auto& modNamePtr : synthPtr->modPtrs) {
    auto& [modName, modPtr] = modNamePtr;
    nodes.emplace_back(NodeEditorNode {
      .modPtr = modPtr,
      .position = glm::vec2(100.0f, 100.0f),
    });
  }
}

void NodeEditorModel::computeLayout() {
    if (!synthPtr) return;

    // Initialize and run layout to completion
    layoutEngine.initialize(synthPtr);
    layoutEngine.compute();

    // Apply positions to nodes for imnodes
    for (auto& node : nodes) {
        glm::vec2 pos = layoutEngine.getNodePosition(node.modPtr);
        node.position = pos;
        ImNodes::SetNodeGridSpacePos(node.modPtr->getId(), ImVec2(pos.x, pos.y));
    }
    
    layoutInitialized = true;
}

void NodeEditorModel::computeLayoutAnimated() {
    if (!synthPtr) return;
    
    if (!layoutInitialized) {
        layoutEngine.initialize(synthPtr);
        layoutInitialized = true;
    }

    // Single step for animated layout
    if (layoutEngine.step()) {
        // Still animating, apply current positions
        for (auto& node : nodes) {
            glm::vec2 pos = layoutEngine.getNodePosition(node.modPtr);
            node.position = pos;
            ImNodes::SetNodeGridSpacePos(node.modPtr->getId(), ImVec2(pos.x, pos.y));
        }
    }
}

bool NodeEditorModel::isLayoutAnimating() const {
    return layoutInitialized && !layoutEngine.isStable();
}

void NodeEditorModel::resetLayout() {
    layoutInitialized = false;
}

void NodeEditorModel::syncPositionsFromImNodes() {
    // Read current positions from imnodes and update our model
    for (auto& node : nodes) {
        ImVec2 imnodesPos = ImNodes::GetNodeGridSpacePos(node.modPtr->getId());
        node.position = glm::vec2(imnodesPos.x, imnodesPos.y);
    }
}

bool NodeEditorModel::saveLayout() const {
    if (!synthPtr) {
        return false;
    }
    return NodeEditorLayoutSerializer::save(*this, synthPtr->name);
}

bool NodeEditorModel::loadLayout() {
    if (!synthPtr) {
        return false;
    }
    return NodeEditorLayoutSerializer::load(*this, synthPtr->name);
}

bool NodeEditorModel::hasStoredLayout() const {
    if (!synthPtr) {
        return false;
    }
    return NodeEditorLayoutSerializer::exists(synthPtr->name);
}


} // namespace ofxMarkSynth
