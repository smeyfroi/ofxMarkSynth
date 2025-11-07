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
  int nodeId; // imnodes node ID
  std::shared_ptr<Mod> modPtr;
  glm::vec2 position;
  std::vector<int> inputPinIds;
  std::vector<int> outputPinIds;
};

struct NodeEditorLink {
  int linkId;
  int startPinId;  // output pin
  int endPinId;    // input pin
  std::string sourceModName;
  int sourceId;
  std::string sinkModName;
  int sinkId;
};

class NodeEditorModel {
public:
  void buildFromSynth(const std::shared_ptr<Synth> synthPtr);
  
  std::shared_ptr<Synth> synthPtr;
  std::vector<NodeEditorNode> nodes;
  std::vector<NodeEditorLink> links;
  
private:
  int nextNodeId = 1;
  int nextPinId = 10000;
  int nextLinkId = 20000;
};



} // namespace ofxMarkSynth
