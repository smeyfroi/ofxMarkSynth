//
//  NodeEditorModel.hpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 07/11/2025.
//

#pragma once

#include <string>
#include <vector>
#include "glm/vec2.hpp"
#include "Mod.hpp"



namespace ofxMarkSynth {



class Synth;

struct NodeEditorNode {
  std::shared_ptr<Mod> modPtr;
  glm::vec2 position;
};

class NodeEditorModel {
public:
  void buildFromSynth(const std::shared_ptr<Synth> synthPtr);
  std::shared_ptr<Synth> synthPtr;
  std::vector<NodeEditorNode> nodes;
};



} // namespace ofxMarkSynth
