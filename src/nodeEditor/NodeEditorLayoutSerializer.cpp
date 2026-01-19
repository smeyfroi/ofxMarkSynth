//
//  NodeEditorLayoutSerializer.cpp
//  ofxMarkSynth
//
//  Created by AI Assistant on 08/11/2025.
//

#include "nodeEditor/NodeEditorLayoutSerializer.hpp"
#include "nodeEditor/NodeEditorModel.hpp"
#include "core/Synth.hpp"
#include "core/Mod.hpp"
#include "imnodes.h"
#include "ofLog.h"
#include "ofUtils.h"
#include <fstream>
#include <filesystem>



namespace ofxMarkSynth {




constexpr const char* LAYOUT_FOLDER_NAME = "node-layout";

static std::string getConfigBasenameKey(const std::string& configPath, const std::string& synthName) {
  if (!configPath.empty()) {
    std::filesystem::path p(configPath);
    auto stem = p.stem().string();
    if (!stem.empty()) return stem;
  }
  return synthName;
}

static std::string getPreferredLayoutFilePath(const std::string& synthName, const std::string& configPath) {
  const std::string key = getConfigBasenameKey(configPath, synthName);
  return Synth::saveConfigFilePath(std::string(LAYOUT_FOLDER_NAME) + "/" + key + "/layout.json");
}




std::string NodeEditorLayoutSerializer::getLayoutFilePath(const std::string& synthName,
                                                    const std::string& configPath) {
  return getPreferredLayoutFilePath(synthName, configPath);
}

bool NodeEditorLayoutSerializer::exists(const std::string& synthName,
                                    const std::string& configPath) {
  std::string preferred = getPreferredLayoutFilePath(synthName, configPath);
  return std::filesystem::exists(preferred);
}

nlohmann::json NodeEditorLayoutSerializer::toJson(const NodeEditorModel& model) {
  nlohmann::json j;
  
  // Metadata
  j["version"] = "1.0";
  j["synth_name"] = model.synthPtr->name;
  if (model.synthPtr) {
    j["config_basename"] = getConfigBasenameKey(model.synthPtr->getCurrentConfigPath(), model.synthPtr->name);
  }
  j["timestamp"] = ofGetTimestampString();
  
  // Node positions
  j["nodes"] = nlohmann::json::array();
  for (const auto& node : model.nodes) {
    nlohmann::json nodeJson;
    if (const auto modPtr = std::get_if<std::shared_ptr<Mod>>(&node.objectPtr)) {
      nodeJson["type"] = "Mod";
    } else if (const auto layerPtr = std::get_if<std::shared_ptr<DrawingLayer>>(&node.objectPtr)) {
      nodeJson["type"] = "DrawingLayer";
    }
    nodeJson["id"] = node.getId();
    nodeJson["name"] = node.getName();
    nodeJson["position"]["x"] = node.position.x;
    nodeJson["position"]["y"] = node.position.y;
    j["nodes"].push_back(nodeJson);
  }
  
  return j;
}

bool NodeEditorLayoutSerializer::save(const NodeEditorModel& model,
                                       const std::string& synthName,
                                       const std::string& configPath) {
  if (!model.synthPtr) {
    ofLogError("NodeEditorLayoutSerializer") << "No synth in model";
    return false;
  }
  
  try {
    std::string filepath = getPreferredLayoutFilePath(synthName, configPath);

    std::filesystem::path dir = std::filesystem::path(filepath).parent_path();
    std::filesystem::create_directories(dir);
    
    nlohmann::json j = toJson(model);
    std::ofstream file(filepath);
    if (!file.is_open()) {
      ofLogError("NodeEditorLayoutSerializer") << "Failed to open: " << filepath;
      return false;
    }
    
    file << j.dump(2); // Pretty print with 2-space indent
    file.close();
    ofLogNotice("NodeEditorLayoutSerializer") << "Saved layout to: " << filepath;
    return true;
  } catch (const std::exception& e) {
    ofLogError("NodeEditorLayoutSerializer") << "Exception: " << e.what();
    return false;
  }
}

void NodeEditorLayoutSerializer::fromJson(const nlohmann::json& j,
                                          NodeEditorModel& model) {
  if (j.contains("version") && j["version"] != "1.0") {
    ofLogWarning("NodeEditorLayoutSerializer") << "Version mismatch";
  }
  
  // Load node positions
  if (!j.contains("nodes") || !j["nodes"].is_array()) {
    ofLogError("NodeEditorLayoutSerializer") << "Invalid JSON: no nodes array";
    return;
  }
  
  std::unordered_map<std::string, glm::vec2> savedPositions;
  
  for (const auto& nodeJson : j["nodes"]) {
    std::string modName = nodeJson["name"];
    glm::vec2 pos;
    pos.x = nodeJson["position"]["x"];
    pos.y = nodeJson["position"]["y"];
    savedPositions[modName] = pos;
  }
  
  // Apply saved positions to nodes in model
  for (auto& node : model.nodes) {
    std::string name = node.getName();
    int id = node.getId();

    auto it = savedPositions.find(name);
    if (it != savedPositions.end()) {
      node.position = it->second;
      // Also set in imnodes
      ImNodes::SetNodeGridSpacePos(id, ImVec2(it->second.x, it->second.y));
    }
  }
}

bool NodeEditorLayoutSerializer::load(NodeEditorModel& model,
                                       const std::string& synthName,
                                       const std::string& configPath) {
  if (!model.synthPtr) {
    ofLogError("NodeEditorLayoutSerializer") << "No synth in model";
    return false;
  }
  
  std::string filepath = getPreferredLayoutFilePath(synthName, configPath);

  if (!std::filesystem::exists(filepath)) {
    ofLogNotice("NodeEditorLayoutSerializer") << "No layout file: " << filepath;
    return false;
  }
  
  try {
    std::ifstream file(filepath);
    if (!file.is_open()) {
      ofLogError("NodeEditorLayoutSerializer") << "Failed to open: " << filepath;
      return false;
    }
    
    nlohmann::json j;
    file >> j;
    file.close();
    
    if (j.contains("synth_name") && j["synth_name"] != model.synthPtr->name) {
      ofLogWarning("NodeEditorLayoutSerializer")
      << "Synth name mismatch: expected " << model.synthPtr->name
      << ", got " << j["synth_name"];
    }
    
    fromJson(j, model);
    ofLogNotice("NodeEditorLayoutSerializer") << "Loaded layout from: " << filepath;
    return true;
  } catch (const std::exception& e) {
    ofLogError("NodeEditorLayoutSerializer") << "Exception: " << e.what();
    return false;
  }
}



} // namespace ofxMarkSynth
