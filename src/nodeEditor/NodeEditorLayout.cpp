//
//  NodeEditorLayout.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 08/11/2025.
//

#include "nodeEditor/NodeEditorLayout.hpp"
#include "core/Mod.hpp"
#include "core/Synth.hpp"
#include "ofMath.h"
#include <cmath>



namespace ofxMarkSynth {



NodeEditorLayout::NodeEditorLayout(const Config& config_, glm::vec2 bounds_)
: config { config_ },
bounds { bounds_ },
center { bounds_ * 0.5f },
currentIteration { 0 }
{
  nodes.clear();
}

void NodeEditorLayout::addNode(const NodeObjectPtr& nodeObjectPtr) {
  LayoutNode node {
    .objectPtr = nodeObjectPtr,
    .velocity = glm::vec2(0, 0),
  };
  if (const auto modPtrPtr = std::get_if<ModPtr>(&nodeObjectPtr)) {
    bool hasSources = !(*modPtrPtr)->sourceNameIdMap.empty();
    bool hasSinks = !(*modPtrPtr)->sinkNameIdMap.empty();
    if (hasSources && !hasSinks) {
      node.position = glm::vec2(ofRandom(bounds.x * 0.0f, bounds.x * 0.2f),
                                ofRandom(bounds.y * 0.1f, bounds.y * 0.8f));
    } else if (!hasSources && hasSinks) {
      node.position = glm::vec2(ofRandom(bounds.x * 0.2f, bounds.x * 0.5f),
                                ofRandom(bounds.y * 0.1f, bounds.y * 0.8f));
    } else {
      node.position = glm::vec2(ofRandom(bounds.x * 0.5f, bounds.x * 0.9f),
                                ofRandom(bounds.y * 0.1f, bounds.y * 0.8f));
    }
  } else if (auto layerPtrPtr = std::get_if<DrawingLayerPtr>(&nodeObjectPtr)) {
    node.position = glm::vec2(ofRandom(bounds.x * 0.4f, bounds.x * 0.6f),
                              ofRandom(bounds.y * 0.9f, bounds.y * 0.95f));
  } else {
    ofLogError("NodeEditorLayout") << "NodeEditorLayout::addNode: unknown node object type";
    return;
  }
  nodes.emplace(nodeObjectPtr, node);
}

void NodeEditorLayout::compute(int iterations) {
  if (iterations < 0) iterations = config.maxIterations;
  currentIteration = 0;
  
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
  for (auto& [objectPtr, node] : nodes) {
    if (node.isFixed) continue;
    node.velocity = glm::vec2(0, 0);
  }
  
  // 1. Repulsion between all nodes (Coulomb-like)
  for (auto& [objectPtr1, node1] : nodes) {
    if (node1.isFixed) continue;
    
    for (auto& [objectPtr2, node2] : nodes) {
      if (objectPtr1 == objectPtr2) continue;
      
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
  for (const auto& [objectPtr, sourceNode] : nodes) {
    if (const auto modPtrPtr = std::get_if<ModPtr>(&objectPtr)) {

      const auto& modPtr = *modPtrPtr;
      if (!modPtr->connections.empty()) {
        for (const auto& [sourceId, sinksPtr] : modPtr->connections) {
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
              auto& mutableSourceNode = const_cast<LayoutNode&>(nodes.at(modPtr));
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
  }
  
  // 3. Gentle attraction to center (keeps graph from drifting)
  for (auto& [objectPtr, node] : nodes) {
    if (node.isFixed) continue;
    
    glm::vec2 toCenter = center - node.position;
    node.velocity += toCenter * config.centerAttraction.x;
  }
}

void NodeEditorLayout::updatePositions() {
  for (auto& [objectPtr, node] : nodes) {
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
  
  for (const auto& [objectPtr, node] : nodes) {
    if (node.isFixed) continue;
    totalMovement += glm::length(node.velocity);
    movableNodes++;
  }
  
  if (movableNodes == 0) return true;
  
  float avgMovement = totalMovement / movableNodes;
  return avgMovement < config.stopThreshold;
}

glm::vec2 NodeEditorLayout::getNodePosition(const NodeObjectPtr& objectPtr) const {
  auto it = nodes.find(objectPtr);
  if (it == nodes.end()) return glm::vec2(0, 0);
  return it->second.position;
}

void NodeEditorLayout::setNodePosition(const NodeObjectPtr& objectPtr, glm::vec2 pos) {
  auto it = nodes.find(objectPtr);
  if (it != nodes.end()) {
    it->second.position = pos;
  }
}

void NodeEditorLayout::pinNode(const NodeObjectPtr& objectPtr, bool fixed) {
  auto it = nodes.find(objectPtr);
  if (it != nodes.end()) {
    it->second.isFixed = fixed;
  }
}



} // namespace ofxMarkSynth
