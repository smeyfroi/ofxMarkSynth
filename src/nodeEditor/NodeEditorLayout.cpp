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
#include <algorithm>



namespace ofxMarkSynth {

NodeEditorLayout::NodeEditorLayout(const Config& config_, glm::vec2 bounds_)
: config { config_ },
  bounds { bounds_ },
  center { bounds_ * 0.5f },
  currentIteration { 0 }
{
  nodes.clear();
  nodeOrder.clear();
}

void NodeEditorLayout::initialize(const std::shared_ptr<Synth> synthPtr) {
  (void)synthPtr; // reserved for future use (e.g., explicit IO classification)

  enum class ModRole { SOURCE, PROCESS, SINK };

  struct ModEntry {
    ModPtr modPtr;
    ModRole role;
  };

  std::vector<ModEntry> mods;
  std::vector<DrawingLayerPtr> layers;
  mods.reserve(nodeOrder.size());
  layers.reserve(nodeOrder.size());

  for (const auto& objectPtr : nodeOrder) {
    if (const auto modPtrPtr = std::get_if<ModPtr>(&objectPtr)) {
      const auto& modPtr = *modPtrPtr;
      bool hasSources = !modPtr->sourceNameIdMap.empty();
      bool hasSinks = !modPtr->sinkNameIdMap.empty();

      ModRole role = ModRole::PROCESS;
      if (hasSources && !hasSinks) {
        role = ModRole::SOURCE;
      } else if (hasSinks && !hasSources) {
        role = ModRole::SINK;
      }

      mods.push_back(ModEntry { modPtr, role });
    } else if (const auto layerPtrPtr = std::get_if<DrawingLayerPtr>(&objectPtr)) {
      layers.push_back(*layerPtrPtr);
    }
  }

  std::stable_sort(mods.begin(), mods.end(), [](const ModEntry& a, const ModEntry& b) {
    return a.modPtr->getName() < b.modPtr->getName();
  });
  std::stable_sort(layers.begin(), layers.end(), [](const DrawingLayerPtr& a, const DrawingLayerPtr& b) {
    return a->name < b->name;
  });

  const float margin = 120.0f;
  const float xLeft = bounds.x * 0.18f;
  const float xMid = bounds.x * 0.50f;
  const float xRight = bounds.x * 0.82f;

  std::vector<ModEntry> sources;
  std::vector<ModEntry> processes;
  std::vector<ModEntry> sinks;
  sources.reserve(mods.size());
  processes.reserve(mods.size());
  sinks.reserve(mods.size());

  for (const auto& m : mods) {
    if (m.role == ModRole::SOURCE) {
      sources.push_back(m);
    } else if (m.role == ModRole::SINK) {
      sinks.push_back(m);
    } else {
      processes.push_back(m);
    }
  }

  auto placeBand = [&](const std::vector<ModEntry>& band, float x) {
    if (band.empty()) return;

    float available = std::max(1.0f, bounds.y - 2.0f * margin);
    float spacing = std::min(190.0f, available / (static_cast<float>(band.size()) + 1.0f));

    for (size_t i = 0; i < band.size(); ++i) {
      const auto& modPtr = band[i].modPtr;
      auto it = nodes.find(modPtr);
      if (it == nodes.end()) continue;

      float y = margin + spacing * (static_cast<float>(i) + 1.0f);
      y = ofClamp(y, margin, bounds.y - margin);

      it->second.position = glm::vec2(x, y);
      it->second.velocity = glm::vec2(0.0f);
      it->second.anchorX = x;
      it->second.useAnchorX = true;
    }
  };

  placeBand(sources, xLeft);
  placeBand(processes, xMid);
  placeBand(sinks, xRight);

  // Deterministic placement for layers: keep them near the mods that use them.
  std::unordered_map<DrawingLayerPtr, std::vector<ModPtr>> layerUsers;
  layerUsers.reserve(layers.size());

  for (const auto& m : mods) {
    const auto& modPtr = m.modPtr;
    for (const auto& [layerName, layerPtrs] : modPtr->namedDrawingLayerPtrs) {
      (void)layerName;
      for (const auto& layerPtr : layerPtrs) {
        layerUsers[layerPtr].push_back(modPtr);
      }
    }
  }

  int layerIndex = 0;
  for (const auto& layerPtr : layers) {
    auto it = nodes.find(layerPtr);
    if (it == nodes.end()) continue;

    float x = bounds.x * 0.50f;
    float y = bounds.y - margin;

    auto usersIt = layerUsers.find(layerPtr);
    if (usersIt != layerUsers.end() && !usersIt->second.empty()) {
      float sumX = 0.0f;
      float avgY = 0.0f;
      float maxY = 0.0f;
      int count = 0;

      for (const auto& modPtr : usersIt->second) {
        auto modIt = nodes.find(modPtr);
        if (modIt == nodes.end()) continue;

        sumX += modIt->second.position.x;
        avgY += modIt->second.position.y;
        maxY = std::max(maxY, modIt->second.position.y);
        count++;
      }

      if (count > 0) {
        float avgX = sumX / static_cast<float>(count);
        avgY = avgY / static_cast<float>(count);

        // Small deterministic spread to reduce overlap when layers share users.
        float spread = static_cast<float>((layerIndex % 5) - 2) * 35.0f;
        x = avgX + spread;

        // Place layers slightly below their user cluster.
        y = std::max(avgY, maxY) + config.layerYOffset;
      }
    }

    x = ofClamp(x, margin, bounds.x - margin);
    y = ofClamp(y, margin, bounds.y - margin);

    it->second.position = glm::vec2(x, y);
    it->second.velocity = glm::vec2(0.0f);
    it->second.anchorX = x;
    it->second.useAnchorX = true;

    layerIndex++;
  }
}

void NodeEditorLayout::addNode(const NodeObjectPtr& nodeObjectPtr) {
  LayoutNode node {
    .objectPtr = nodeObjectPtr,
    .position = center,
    .velocity = glm::vec2(0, 0),
  };

  // Deterministic initial placement is handled by initialize().
  nodes.emplace(nodeObjectPtr, node);
  nodeOrder.push_back(nodeObjectPtr);
}



void NodeEditorLayout::randomize() {
  for (const auto& objectPtr : nodeOrder) {
    auto& node = nodes.at(objectPtr);
    if (node.isFixed) continue;

    if (const auto modPtrPtr = std::get_if<ModPtr>(&objectPtr)) {
      bool hasSources = !(*modPtrPtr)->sourceNameIdMap.empty();
      bool hasSinks = !(*modPtrPtr)->sinkNameIdMap.empty();
      if (hasSources && !hasSinks) {
        node.position = glm::vec2(ofRandom(bounds.x * 0.0f, bounds.x * 0.25f), ofRandom(bounds.y * 0.1f, bounds.y * 0.85f));
      } else if (!hasSources && hasSinks) {
        node.position = glm::vec2(ofRandom(bounds.x * 0.70f, bounds.x * 0.95f), ofRandom(bounds.y * 0.1f, bounds.y * 0.85f));
      } else {
        node.position = glm::vec2(ofRandom(bounds.x * 0.25f, bounds.x * 0.70f), ofRandom(bounds.y * 0.1f, bounds.y * 0.85f));
      }
      node.useAnchorX = false;
    } else if (auto layerPtrPtr = std::get_if<DrawingLayerPtr>(&objectPtr)) {
      node.position = glm::vec2(ofRandom(bounds.x * 0.25f, bounds.x * 0.75f), ofRandom(bounds.y * 0.75f, bounds.y * 0.95f));
      node.useAnchorX = false;
    }

    node.velocity = glm::vec2(0.0f);
  }
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
  // Reset velocities
  for (const auto& objectPtr : nodeOrder) {
    auto& node = nodes.at(objectPtr);
    if (node.isFixed) continue;
    node.velocity = glm::vec2(0, 0);
  }
  
  applyRepulsionForces();
  applySpringForces();
  applyBandAttraction();
  applyCenterAttraction();
}

void NodeEditorLayout::applyRepulsionForces() {
  // Coulomb-like repulsion between all nodes (deterministic iteration).
  for (size_t i = 0; i < nodeOrder.size(); ++i) {
    const auto& objectPtr1 = nodeOrder[i];
    auto it1 = nodes.find(objectPtr1);
    if (it1 == nodes.end()) continue;
    auto& node1 = it1->second;
    if (node1.isFixed) continue;

    for (size_t j = 0; j < nodeOrder.size(); ++j) {
      if (i == j) continue;
      const auto& objectPtr2 = nodeOrder[j];
      auto it2 = nodes.find(objectPtr2);
      if (it2 == nodes.end()) continue;
      const auto& node2 = it2->second;

      glm::vec2 delta = node1.position - node2.position;
      float distance = glm::length(delta);

      if (distance < 1.0f) distance = 1.0f;  // avoid singularity

      // Repulsion force: F = k / r^2
      float force = config.repulsionStrength / (distance * distance);
      glm::vec2 direction = glm::normalize(delta);
      node1.velocity += direction * force;
    }
  }
}

void NodeEditorLayout::applySpringForces() {
  // Spring attraction along links (connections + mod-to-layer assignments).
  for (const auto& objectPtr : nodeOrder) {
    const auto modPtrPtr = std::get_if<ModPtr>(&objectPtr);
    if (!modPtrPtr) continue;

    const auto& modPtr = *modPtrPtr;
    auto sourceIt = nodes.find(modPtr);
    if (sourceIt == nodes.end()) continue;

    auto& sourceNode = sourceIt->second;

    // Mod -> Mod connections (sort keys for determinism)
    std::vector<int> sourceIds;
    sourceIds.reserve(modPtr->connections.size());
    for (const auto& [sourceId, sinksPtr] : modPtr->connections) {
      (void)sinksPtr;
      sourceIds.push_back(sourceId);
    }
    std::sort(sourceIds.begin(), sourceIds.end());

    for (int sourceId : sourceIds) {
      const auto& sinksPtr = modPtr->connections.at(sourceId);
      if (!sinksPtr) continue;

      for (const auto& [sinkModPtr, sinkId] : *sinksPtr) {
        (void)sinkId;
        auto sinkIt = nodes.find(sinkModPtr);
        if (sinkIt == nodes.end()) continue;

        auto& sinkNode = sinkIt->second;

        glm::vec2 delta = sinkNode.position - sourceNode.position;
        float distance = glm::length(delta);
        if (distance < 0.1f) continue;

        float displacement = distance - config.springLength;
        glm::vec2 force = delta * (config.springStrength * displacement / distance);

        if (!sourceNode.isFixed) sourceNode.velocity += force;
        if (!sinkNode.isFixed) sinkNode.velocity -= force;
      }
    }

    // Mod -> DrawingLayer relationships (so layer nodes participate).
    for (const auto& [layerName, layerPtrs] : modPtr->namedDrawingLayerPtrs) {
      auto currentLayerPtrOpt = modPtr->getCurrentNamedDrawingLayerPtr(layerName);

      for (const auto& layerPtr : layerPtrs) {
        auto layerIt = nodes.find(layerPtr);
        if (layerIt == nodes.end()) continue;

        auto& layerNode = layerIt->second;

        glm::vec2 delta = layerNode.position - sourceNode.position;
        float distance = glm::length(delta);
        if (distance < 0.1f) continue;

        float strength = config.layerSpringStrength;
        if (currentLayerPtrOpt && (*currentLayerPtrOpt)->id == layerPtr->id) {
          strength *= 2.0f; // bias the active layer closer
        }

        float displacement = distance - config.layerSpringLength;
        glm::vec2 force = delta * (strength * displacement / distance);

        if (!sourceNode.isFixed) sourceNode.velocity += force;
        if (!layerNode.isFixed) layerNode.velocity -= force;
      }
    }
  }
}

void NodeEditorLayout::applyBandAttraction() {
  // Light pull toward deterministic X "bands" (left-to-right semantics).
  if (config.bandAttractionStrength <= 0.0f) return;

  for (const auto& objectPtr : nodeOrder) {
    auto& node = nodes.at(objectPtr);
    if (node.isFixed || !node.useAnchorX) continue;

    float dx = node.anchorX - node.position.x;
    node.velocity.x += dx * config.bandAttractionStrength;
  }
}

void NodeEditorLayout::applyCenterAttraction() {
  // Gentle attraction to center (keeps graph from drifting)
  for (const auto& objectPtr : nodeOrder) {
    auto& node = nodes.at(objectPtr);
    if (node.isFixed) continue;
    
    glm::vec2 toCenter = center - node.position;
    node.velocity += toCenter * config.centerAttraction.x;
  }
}

void NodeEditorLayout::updatePositions() {
  for (const auto& objectPtr : nodeOrder) {
    auto& node = nodes.at(objectPtr);
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
  
  for (const auto& objectPtr : nodeOrder) {
    const auto& node = nodes.at(objectPtr);
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
