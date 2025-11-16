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
#include "Mod.hpp"



namespace ofxMarkSynth {



class Synth;

class SynthConfigSerializer {
  using NamedLayers = std::unordered_map<std::string, DrawingLayerPtr>;
  
public:
  // Load Synth configuration from JSON file
  static bool load(std::shared_ptr<Synth> synth,
                   const std::filesystem::path& filepath,
                   const ResourceManager& resources = ResourceManager{});
  
  // Check if config file exists
  static bool exists(const std::filesystem::path& filepath);
  
  // Get default config directory path
  static std::string getConfigDirectory(const std::string& synthName);
  
  // Get config file path by name
  static std::string getConfigFilePath(const std::string& synthName, const std::string& configName);
  
private:
  // Parse JSON and populate Synth
  static bool fromJson(const nlohmann::json& j, std::shared_ptr<Synth> synth, const ResourceManager& resources);
  
  // Helper functions for parsing specific sections
  static NamedLayers parseDrawingLayers(const nlohmann::json& j, std::shared_ptr<Synth> synth);
  static bool parseMods(const nlohmann::json& j, std::shared_ptr<Synth> synth, const ResourceManager& resources, const NamedLayers& layers);
  static bool parseConnections(const nlohmann::json& j, std::shared_ptr<Synth> synth);
  static bool parseIntents(const nlohmann::json& j, std::shared_ptr<Synth> synth);
  
  // Helper to convert JSON string to GL enum
  static int glEnumFromString(const std::string& str);
  static int ofBlendModeFromString(const std::string& str);
};



} // namespace ofxMarkSynth
