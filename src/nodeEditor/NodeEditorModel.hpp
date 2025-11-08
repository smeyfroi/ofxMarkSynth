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
#include "NodeEditorLayout.hpp"



namespace ofxMarkSynth {



class Synth;

struct NodeEditorNode {
  std::shared_ptr<Mod> modPtr;
  glm::vec2 position;
};

class NodeEditorModel {
public:
  void buildFromSynth(const std::shared_ptr<Synth> synthPtr);
  
  // Layout methods
  void computeLayout();               // Compute layout instantly
  void computeLayoutAnimated();       // Single step for animated layout
  bool isLayoutAnimating() const;     // Check if still animating
  void resetLayout();                 // Reset to trigger new layout
  
  // Save/load methods
  void syncPositionsFromImNodes();    // Sync positions from imnodes to model
  bool saveLayout() const;            // Save layout to file
  bool loadLayout();                  // Load layout from file
  bool hasStoredLayout() const;       // Check if layout file exists
  
  std::shared_ptr<Synth> synthPtr;
  std::vector<NodeEditorNode> nodes;

private:
    NodeEditorLayout layoutEngine;
    bool layoutInitialized { false };
};



} // namespace ofxMarkSynth
