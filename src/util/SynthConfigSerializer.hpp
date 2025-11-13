//
//  SynthConfigSerializer.hpp
//  ofxMarkSynth
//
//  Created for config serialization system
//

#pragma once

#include <string>
#include "nlohmann/json.hpp"
#include "glm/vec2.hpp"
#include "ModFactory.hpp"



namespace ofxMarkSynth {



class Synth;

class SynthConfigSerializer {
public:
  // Load Synth configuration from JSON file
  static bool load(Synth* synth,
                   const std::string& filepath,
                   const ResourceMap& resources = ResourceMap{});
  
  // Check if config file exists
  static bool exists(const std::string& filepath);
  
  // Get default config directory path
  static std::string getConfigDirectory(const std::string& synthName);
  
  // Get config file path by name
  static std::string getConfigFilePath(const std::string& synthName, const std::string& configName);
  
private:
  // Parse JSON and populate Synth
  static bool fromJson(const nlohmann::json& j, Synth* synth, const ResourceMap& resources);
  
  // Helper functions for parsing specific sections
  static bool parseDrawingLayers(const nlohmann::json& j, Synth* synth);
  static bool parseMods(const nlohmann::json& j, Synth* synth, const ResourceMap& resources);
  static bool parseConnections(const nlohmann::json& j, Synth* synth);
  static bool parseIntents(const nlohmann::json& j, Synth* synth);
  
  // Helper to convert JSON string to GL enum
  static int glEnumFromString(const std::string& str);
  static int ofBlendModeFromString(const std::string& str);
};



} // namespace ofxMarkSynth
