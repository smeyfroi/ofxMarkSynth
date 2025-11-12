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

using NodeObjectPtr = std::variant<ModPtr, DrawingLayerPtr>;

static constexpr std::string emptyString = "";

struct NodeEditorNode {
  NodeObjectPtr objectPtr;
  glm::vec2 position;
  int getId() const {
    if (const auto modPtrPtr = std::get_if<ModPtr>(&objectPtr)) {
      return (*modPtrPtr)->getId();
    } else if (const auto drawingLayerPtrPtr = std::get_if<DrawingLayerPtr>(&objectPtr)) {
      return (*drawingLayerPtrPtr)->id;
    } else {
      ofLogError() << "NodeEditorNode::getId() with unknown objectPtr type";
      return -1;
    }
  };
  const std::string& getName() const {
    if (const auto modPtrPtr = std::get_if<ModPtr>(&objectPtr)) {
      return (*modPtrPtr)->getName();
    } else if (const auto drawingLayerPtrPtr = std::get_if<DrawingLayerPtr>(&objectPtr)) {
      return (*drawingLayerPtrPtr)->name;
    } else {
      ofLogError() << "NodeEditorNode::getName() with unknown objectPtr type";
      return emptyString;
    }
  };
};

class NodeEditorModel {
public:
  void buildFromSynth(const std::shared_ptr<Synth> synthPtr);
  
  void computeLayout();
  void computeLayoutAnimated();
  bool isLayoutAnimating() const;
  void resetLayout();
  
  void syncPositionsFromImNodes();
  bool saveLayout() const;
  bool loadLayout();
  bool hasStoredLayout() const;
  
  std::shared_ptr<Synth> synthPtr;
  std::vector<NodeEditorNode> nodes;
  
  static int sinkId(int modId, int sinkId) { return modId + sinkId; }
  static int sourceId(int modId, int sourceId) { return modId + sourceId + 100000; }

private:
  std::unique_ptr<NodeEditorLayout> layoutEnginePtr;
};



} // namespace ofxMarkSynth
