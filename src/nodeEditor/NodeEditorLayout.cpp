//
//  NodeEditorLayout.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 08/11/2025.
//

#include "NodeEditorLayout.hpp"
#include "Mod.hpp"
#include "Synth.hpp"
#include "ofMath.h"
#include <cmath>



namespace ofxMarkSynth {



NodeEditorLayout::NodeEditorLayout(const Config& config_)
: config(config_)
{}

void NodeEditorLayout::initialize(const std::shared_ptr<Synth> synthPtr, glm::vec2 bounds_) {
  bounds = bounds_;
  center = bounds * 0.5f;
  nodes.clear();
  currentIteration = 0;
  
  // Create nodes with random initial positions from Synth's modPtrs
  for (const auto& [name, modPtr] : synthPtr->modPtrs) {
    LayoutNode node;
    node.modPtr = modPtr;
    // Random position in central 50% of bounds
    node.position = glm::vec2(
                              ofRandom(bounds.x * 0.25f, bounds.x * 0.75f),
                              ofRandom(bounds.y * 0.25f, bounds.y * 0.75f)
                              );
    node.velocity = glm::vec2(0, 0);
    nodes[modPtr] = node;
  }
}

void NodeEditorLayout::compute(int iterations) {
  if (iterations < 0) iterations = config.maxIterations;
  
  for (int i = 0; i < iterations; ++i) {
    if (!step()) break;  // early exit if stable
  }
}

bool NodeEditorLayout::step() {
  if (currentIteration >= config.maxIterations) return false;
  
  applyForces();
  updatePositions();
  
  currentIteration++;
  return !isStable();
}

void NodeEditorLayout::applyForces() {
  // Reset forces (stored in velocity for this iteration)
  for (auto& [modPtr, node] : nodes) {
    if (node.isFixed) continue;
    node.velocity = glm::vec2(0, 0);
  }
  
  // 1. Repulsion between all nodes (Coulomb-like)
  for (auto& [modPtr1, node1] : nodes) {
    if (node1.isFixed) continue;
    
    for (auto& [modPtr2, node2] : nodes) {
      if (modPtr1 == modPtr2) continue;
      
      glm::vec2 delta = node1.position - node2.position;
      float distance = glm::length(delta);
      
      if (distance < 1.0f) distance = 1.0f;  // avoid singularity
      
      // Repulsion force: F = k / r^2
      float force = config.repulsionStrength / (distance * distance);
      glm::vec2 direction = glm::normalize(delta);
      node1.velocity += direction * force;
    }
  }
  
  // 2. Spring attraction along links (connections)
  // Iterate through all nodes and their connections
  for (const auto& [sourceModPtr, sourceNode] : nodes) {
    if (!sourceNode.modPtr->connections.empty()) {
      for (const auto& [sourceId, sinksPtr] : sourceNode.modPtr->connections) {
        if (!sinksPtr) continue;
        
        for (const auto& [sinkModPtr, sinkId] : *sinksPtr) {
          auto sinkIt = nodes.find(sinkModPtr);
          if (sinkIt == nodes.end()) continue;
          
          auto& sinkNode = sinkIt->second;
          
          glm::vec2 delta = sinkNode.position - sourceNode.position;
          float distance = glm::length(delta);
          
          if (distance < 0.1f) continue;  // too close
          
          // Spring force: F = k * (distance - restLength)
          float displacement = distance - config.springLength;
          glm::vec2 force = delta * (config.springStrength * displacement / distance);
          
          // Apply force to both nodes (non-const access needed)
          if (!sourceNode.isFixed) {
            auto& mutableSourceNode = const_cast<LayoutNode&>(nodes.at(sourceModPtr));
            mutableSourceNode.velocity += force;
          }
          if (!sinkNode.isFixed) {
            auto& mutableSinkNode = const_cast<LayoutNode&>(nodes.at(sinkModPtr));
            mutableSinkNode.velocity -= force;
          }
        }
      }
    }
  }
  
  // 3. Gentle attraction to center (keeps graph from drifting)
  for (auto& [modPtr, node] : nodes) {
    if (node.isFixed) continue;
    
    glm::vec2 toCenter = center - node.position;
    node.velocity += toCenter * config.centerAttraction.x;
  }
}

void NodeEditorLayout::updatePositions() {
  for (auto& [modPtr, node] : nodes) {
    if (node.isFixed) continue;
    
    // Cap velocity
    float speed = glm::length(node.velocity);
    if (speed > config.maxSpeed) {
      node.velocity = glm::normalize(node.velocity) * config.maxSpeed;
    }
    
    // Update position
    node.position += node.velocity;
    
    // Apply damping for next iteration
    node.velocity *= config.damping;
    
    // Keep within bounds with soft boundary
    const float margin = 50.0f;
    if (node.position.x < margin) {
      node.position.x = margin;
      node.velocity.x *= -0.5f;
    }
    if (node.position.x > bounds.x - margin) {
      node.position.x = bounds.x - margin;
      node.velocity.x *= -0.5f;
    }
    if (node.position.y < margin) {
      node.position.y = margin;
      node.velocity.y *= -0.5f;
    }
    if (node.position.y > bounds.y - margin) {
      node.position.y = bounds.y - margin;
      node.velocity.y *= -0.5f;
    }
  }
}

bool NodeEditorLayout::isStable() const {
  float totalMovement = 0.0f;
  int movableNodes = 0;
  
  for (const auto& [modPtr, node] : nodes) {
    if (node.isFixed) continue;
    totalMovement += glm::length(node.velocity);
    movableNodes++;
  }
  
  if (movableNodes == 0) return true;
  
  float avgMovement = totalMovement / movableNodes;
  return avgMovement < config.stopThreshold;
}

glm::vec2 NodeEditorLayout::getNodePosition(const ModPtr& modPtr) const {
  auto it = nodes.find(modPtr);
  if (it == nodes.end()) return glm::vec2(0, 0);
  return it->second.position;
}

void NodeEditorLayout::setNodePosition(const ModPtr& modPtr, glm::vec2 pos) {
  auto it = nodes.find(modPtr);
  if (it != nodes.end()) {
    it->second.position = pos;
  }
}

void NodeEditorLayout::pinNode(const ModPtr& modPtr, bool fixed) {
  auto it = nodes.find(modPtr);
  if (it != nodes.end()) {
    it->second.isFixed = fixed;
  }
}

void NodeEditorLayout::randomize() {
  currentIteration = 0;
  for (auto& [modPtr, node] : nodes) {
    if (node.isFixed) continue;
    node.position = glm::vec2(ofRandom(bounds.x * 0.25f, bounds.x * 0.75f),
                              ofRandom(bounds.y * 0.25f, bounds.y * 0.75f));
    node.velocity = glm::vec2(0, 0);
  }
}



} // namespace ofxMarkSynth
