//
//  SynthConfigSerializer.hpp
//  ofxMarkSynth
//
//  Created for config serialization system
//

#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include "nlohmann/json.hpp"
#include "glm/vec2.hpp"
#include "config/ModFactory.hpp"
#include "core/Mod.hpp"
#include "util/OrderedMap.h"



namespace ofxMarkSynth {



class Synth;

class SynthConfigSerializer {
  using NamedLayers = OrderedMap<std::string, DrawingLayerPtr>;
  
public:
  // Load Synth configuration from JSON file
  static bool load(std::shared_ptr<Synth> synth,
                   const std::filesystem::path& filepath,
                   const ResourceManager& resources = ResourceManager{});
  
  // Check if config file exists
  static bool exists(const std::filesystem::path& filepath);
  
private:
  // Use ordered_json to preserve key order from config files
  using OrderedJson = nlohmann::ordered_json;
  
  // Parse JSON and populate Synth
  static bool fromJson(const OrderedJson& j, std::shared_ptr<Synth> synth, const ResourceManager& resources, const std::string& synthName);
  
  // Helper functions for parsing specific sections
  static NamedLayers parseDrawingLayers(const OrderedJson& j, std::shared_ptr<Synth> synth);
  static bool parseMods(const OrderedJson& j, std::shared_ptr<Synth> synth, const ResourceManager& resources, const NamedLayers& layers);
  static bool parseConnections(const OrderedJson& j, std::shared_ptr<Synth> synth);
  static bool parseIntents(const OrderedJson& j, std::shared_ptr<Synth> synth);
  static bool parseSynthConfig(const OrderedJson& j, std::shared_ptr<Synth> synth);
  
  // Helper to convert JSON string to GL enum
  static int glEnumFromString(const std::string& str);
  static int ofBlendModeFromString(const std::string& str);
};



} // namespace ofxMarkSynth
