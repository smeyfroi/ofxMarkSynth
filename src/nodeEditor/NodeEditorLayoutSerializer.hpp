//
//  NodeEditorLayoutSerializer.hpp
//  ofxMarkSynth
//
//  Created by AI Assistant on 08/11/2025.
//

#pragma once

#include <string>
#include "nlohmann/json.hpp"
#include "glm/vec2.hpp"



namespace ofxMarkSynth {



class Synth;
class NodeEditorModel;

class NodeEditorLayoutSerializer {
public:
    // Save layout to file
    static bool save(const NodeEditorModel& model,
                   const std::string& synthName,
                   const std::string& configPath);
    
    // Load layout from file
    static bool load(NodeEditorModel& model,
                   const std::string& synthName,
                   const std::string& configPath);
    
    // Check if layout file exists
    // TEMPORARY: also checks legacy synth-name layouts for backward compatibility.
    static bool exists(const std::string& synthName,
                     const std::string& configPath);
    
    // Get file path for synth
    static std::string getLayoutFilePath(const std::string& synthName,
                                      const std::string& configPath);
    
private:
    // Serialize to JSON
    static nlohmann::json toJson(const NodeEditorModel& model);
    
    // Deserialize from JSON
    static void fromJson(const nlohmann::json& j, NodeEditorModel& model);
};



} // namespace ofxMarkSynth
