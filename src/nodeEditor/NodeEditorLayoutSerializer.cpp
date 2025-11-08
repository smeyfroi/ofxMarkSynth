//
//  NodeEditorLayoutSerializer.cpp
//  ofxMarkSynth
//
//  Created by AI Assistant on 08/11/2025.
//

#include "NodeEditorLayoutSerializer.hpp"
#include "NodeEditorModel.hpp"
#include "Synth.hpp"
#include "Mod.hpp"
#include "imnodes.h"
#include "ofLog.h"
#include "ofUtils.h"
#include <fstream>
#include <filesystem>



namespace ofxMarkSynth {



// Use existing saveFilePath helper from Synth.cpp
extern std::string saveFilePath(const std::string& filename);
constexpr const char* LAYOUT_FOLDER_NAME = "node-layouts";



std::string NodeEditorLayoutSerializer::getLayoutFilePath(const std::string& synthName) {
    return saveFilePath(std::string(LAYOUT_FOLDER_NAME) + "/" + synthName + "/layout.json");
}

bool NodeEditorLayoutSerializer::exists(const std::string& synthName) {
    std::string path = getLayoutFilePath(synthName);
    return std::filesystem::exists(path);
}

nlohmann::json NodeEditorLayoutSerializer::toJson(const NodeEditorModel& model) {
    nlohmann::json j;
    
    // Metadata
    j["version"] = "1.0";
    j["synth_name"] = model.synthPtr->name;
    j["timestamp"] = ofGetTimestampString();
    
    // Canvas size (stored for validation on load)
    j["canvas_size"]["width"] = 1200;
    j["canvas_size"]["height"] = 800;
    
    // Node positions
    j["nodes"] = nlohmann::json::array();
    for (const auto& node : model.nodes) {
        nlohmann::json nodeJson;
        nodeJson["mod_name"] = node.modPtr->name;
        nodeJson["mod_id"] = node.modPtr->getId();
        nodeJson["position"]["x"] = node.position.x;
        nodeJson["position"]["y"] = node.position.y;
        j["nodes"].push_back(nodeJson);
    }
    
    return j;
}

bool NodeEditorLayoutSerializer::save(const NodeEditorModel& model, 
                                      const std::string& synthName) {
    if (!model.synthPtr) {
        ofLogError("NodeEditorLayoutSerializer") << "No synth in model";
        return false;
    }
    
    try {
        // Create directory if needed
        std::string filepath = getLayoutFilePath(synthName);
        std::filesystem::path dir = std::filesystem::path(filepath).parent_path();
        std::filesystem::create_directories(dir);
        
        // Serialize to JSON
        nlohmann::json j = toJson(model);
        
        // Write to file
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
    // Validate version
    if (j.contains("version") && j["version"] != "1.0") {
        ofLogWarning("NodeEditorLayoutSerializer") << "Version mismatch";
    }
    
    // Load node positions
    if (!j.contains("nodes") || !j["nodes"].is_array()) {
        ofLogError("NodeEditorLayoutSerializer") << "Invalid JSON: no nodes array";
        return;
    }
    
    // Create map of mod_name -> saved position
    std::unordered_map<std::string, glm::vec2> savedPositions;
    
    for (const auto& nodeJson : j["nodes"]) {
        std::string modName = nodeJson["mod_name"];
        glm::vec2 pos;
        pos.x = nodeJson["position"]["x"];
        pos.y = nodeJson["position"]["y"];
        savedPositions[modName] = pos;
    }
    
    // Apply saved positions to nodes in model
    for (auto& node : model.nodes) {
        auto it = savedPositions.find(node.modPtr->name);
        if (it != savedPositions.end()) {
            node.position = it->second;
            
            // Also set in imnodes
            ImNodes::SetNodeGridSpacePos(node.modPtr->getId(), 
                                        ImVec2(it->second.x, it->second.y));
        }
    }
}

bool NodeEditorLayoutSerializer::load(NodeEditorModel& model, 
                                      const std::string& synthName) {
    if (!model.synthPtr) {
        ofLogError("NodeEditorLayoutSerializer") << "No synth in model";
        return false;
    }
    
    std::string filepath = getLayoutFilePath(synthName);
    
    if (!std::filesystem::exists(filepath)) {
        ofLogNotice("NodeEditorLayoutSerializer") << "No layout file: " << filepath;
        return false;
    }
    
    try {
        // Read file
        std::ifstream file(filepath);
        if (!file.is_open()) {
            ofLogError("NodeEditorLayoutSerializer") << "Failed to open: " << filepath;
            return false;
        }
        
        // Parse JSON
        nlohmann::json j;
        file >> j;
        file.close();
        
        // Validate synth name matches
        if (j.contains("synth_name") && j["synth_name"] != model.synthPtr->name) {
            ofLogWarning("NodeEditorLayoutSerializer") 
                << "Synth name mismatch: expected " << model.synthPtr->name 
                << ", got " << j["synth_name"];
        }
        
        // Apply positions
        fromJson(j, model);
        
        ofLogNotice("NodeEditorLayoutSerializer") << "Loaded layout from: " << filepath;
        return true;
        
    } catch (const std::exception& e) {
        ofLogError("NodeEditorLayoutSerializer") << "Exception: " << e.what();
        return false;
    }
}



} // namespace ofxMarkSynth
