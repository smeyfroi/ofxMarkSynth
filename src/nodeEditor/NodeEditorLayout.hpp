//
//  NodeEditorLayout.hpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 08/11/2025.
//

#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include "glm/vec2.hpp"



namespace ofxMarkSynth {



class Mod;
class Synth;
using ModPtr = std::shared_ptr<Mod>;

struct LayoutNode {
  ModPtr modPtr;
  glm::vec2 position;
  glm::vec2 velocity;
  bool isFixed { false };  // for pinning nodes
};

class NodeEditorLayout {
public:
  
//  ### Change Speed
//
//  // Faster animation
//  config.damping = 0.7f;        // Less damping
//  config.maxSpeed = 100.0f;     // Faster movement
//  config.stopThreshold = 1.0f;  // Looser convergence
//
//  // Slower, smoother animation
//  config.damping = 0.95f;       // More damping
//  config.maxSpeed = 30.0f;      // Slower movement
//  config.stopThreshold = 0.1f;  // Tighter convergence
//
//  ### Change Spacing
//
//  // More spread out
//  config.repulsionStrength = 2000.0f;  // Stronger repulsion
//  config.springLength = 300.0f;        // Longer connections
//
//  // More compact
//  config.repulsionStrength = 500.0f;   // Weaker repulsion
//  config.springLength = 150.0f;        // Shorter connections
  
  struct Config {
    float repulsionStrength;
    float springStrength;
    float springLength;
    float damping;
    float maxSpeed;
    float stopThreshold;
    int maxIterations;
    glm::vec2 centerAttraction;
    
    Config()
      : repulsionStrength(3000.0f)
      , springStrength(0.001f)
      , springLength(400.0f)
      , damping(0.85f)
      , maxSpeed(50.0f)
      , stopThreshold(0.5f)
      , maxIterations(300)
      , centerAttraction(0.0001f)
    {}
  };
  
  NodeEditorLayout(const Config& config = Config());
  
  // Initialize directly from Synth
  void initialize(const std::shared_ptr<Synth> synthPtr,
                  glm::vec2 bounds = glm::vec2(1600, 1200));
  
  // Run layout algorithm
  void compute(int iterations = -1);  // -1 = use maxIterations
  
  // Single iteration (for incremental/animated layout)
  bool step();  // returns true if still moving
  
  // Get results
  glm::vec2 getNodePosition(const ModPtr& modPtr) const;
  void setNodePosition(const ModPtr& modPtr, glm::vec2 pos);
  void pinNode(const ModPtr& modPtr, bool fixed = true);
  
  // Reset to random positions
  void randomize();
  
  // Check if layout has converged
  bool isStable() const;
  
  Config config;
  
private:
  void applyForces();
  void updatePositions();
  
  std::unordered_map<ModPtr, LayoutNode> nodes;
  glm::vec2 bounds;
  glm::vec2 center;
  int currentIteration { 0 };
};



} // namespace ofxMarkSynth
