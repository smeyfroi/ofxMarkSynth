//
//  NodeEditorModel.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 07/11/2025.
//

#include "NodeEditorModel.hpp"
#include "Synth.hpp"


namespace ofxMarkSynth {



void NodeEditorModel::buildFromSynth(const std::shared_ptr<Synth> synthPtr_) {
  synthPtr = synthPtr_;
  
  for (const auto& modNamePtr : synthPtr->modPtrs) {
    auto& [modName, modPtr] = modNamePtr;
    nodes.emplace_back(NodeEditorNode {
      .nodeId = nextNodeId++,
      .modPtr = modPtr,
      .position = glm::vec2(100.0f, 100.0f),
    });
  }
  
  // TODO: build connections based on mod connections
}



} // namespace ofxMarkSynth
