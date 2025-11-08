//
//  NodeEditorModel.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 07/11/2025.
//

#include "NodeEditorModel.hpp"
#include "Synth.hpp"
#include "imnodes.h"



namespace ofxMarkSynth {



void NodeEditorModel::buildFromSynth(const std::shared_ptr<Synth> synthPtr_) {
  synthPtr = synthPtr_;
  nodes.clear();
  layoutInitialized = false;
  
  // Build node list from Synth's modPtrs
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


} // namespace ofxMarkSynth
