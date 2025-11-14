//
//  SynthConfigSerializer.cpp
//  ofxMarkSynth
//
//  Created for config serialization system
//

#include "SynthConfigSerializer.hpp"
#include "Synth.hpp"
#include "Intent.hpp"
#include "ofLog.h"
#include "ofUtils.h"
#include <fstream>
#include <filesystem>



namespace ofxMarkSynth {



// Use existing saveFilePath helper from Synth.cpp
extern std::string saveFilePath(const std::string& filename);
constexpr const char* CONFIG_FOLDER_NAME = "configs";



std::string SynthConfigSerializer::getConfigDirectory(const std::string& synthName) {
  return saveFilePath(std::string(CONFIG_FOLDER_NAME) + "/" + synthName);
}

std::string SynthConfigSerializer::getConfigFilePath(const std::string& synthName, const std::string& configName) {
  return saveFilePath(std::string(CONFIG_FOLDER_NAME) + "/" + synthName + "/" + configName + ".json");
}

bool SynthConfigSerializer::exists(const std::filesystem::path& filepath) {
  return std::filesystem::exists(filepath);
}

int SynthConfigSerializer::glEnumFromString(const std::string& str) {
  // Common GL enums used in ofxMarkSynth
  if (str == "GL_RGBA") return GL_RGBA;
  if (str == "GL_RGB") return GL_RGB;
  if (str == "GL_RGBA32F") return GL_RGBA32F;
  if (str == "GL_RGB32F") return GL_RGB32F;
  if (str == "GL_RGBA16F") return GL_RGBA16F;
  if (str == "GL_RGB16F") return GL_RGB16F;
  if (str == "GL_RGBA8") return GL_RGBA8;
  if (str == "GL_RGB8") return GL_RGB8;
  
  // Wrap modes
  if (str == "GL_CLAMP_TO_EDGE") return GL_CLAMP_TO_EDGE;
  if (str == "GL_REPEAT") return GL_REPEAT;
  if (str == "GL_MIRRORED_REPEAT") return GL_MIRRORED_REPEAT;
  
  ofLogWarning("SynthConfigSerializer") << "Unknown GL enum: " << str << ", defaulting to GL_RGBA";
  return GL_RGBA;
}

int SynthConfigSerializer::ofBlendModeFromString(const std::string& str) {
  if (str == "OF_BLENDMODE_DISABLED") return OF_BLENDMODE_DISABLED;
  if (str == "OF_BLENDMODE_ALPHA") return OF_BLENDMODE_ALPHA;
  if (str == "OF_BLENDMODE_ADD") return OF_BLENDMODE_ADD;
  if (str == "OF_BLENDMODE_SUBTRACT") return OF_BLENDMODE_SUBTRACT;
  if (str == "OF_BLENDMODE_MULTIPLY") return OF_BLENDMODE_MULTIPLY;
  if (str == "OF_BLENDMODE_SCREEN") return OF_BLENDMODE_SCREEN;
  
  ofLogWarning("SynthConfigSerializer") << "Unknown blend mode: " << str << ", defaulting to OF_BLENDMODE_ALPHA";
  return OF_BLENDMODE_ALPHA;
}

bool SynthConfigSerializer::parseDrawingLayers(const nlohmann::json& j, std::shared_ptr<Synth> synth) {
  if (!j.contains("drawingLayers") || !j["drawingLayers"].is_object()) {
    ofLogNotice("SynthConfigSerializer") << "No drawingLayers section in config";
    return true; // Not an error - layers are optional
  }
  
  try {
    for (const auto& [name, layerJson] : j["drawingLayers"].items()) {
      glm::vec2 size(1920, 1080); // defaults
      if (layerJson.contains("size") && layerJson["size"].is_array() && layerJson["size"].size() == 2) {
        size.x = layerJson["size"][0];
        size.y = layerJson["size"][1];
      }
      
      GLint internalFormat = GL_RGBA;
      if (layerJson.contains("internalFormat") && layerJson["internalFormat"].is_string()) {
        internalFormat = glEnumFromString(layerJson["internalFormat"]);
      }
      
      int wrap = GL_CLAMP_TO_EDGE;
      if (layerJson.contains("wrap") && layerJson["wrap"].is_string()) {
        wrap = glEnumFromString(layerJson["wrap"]);
      }
      
      float fadeBy = 0.0f;
      if (layerJson.contains("fadeBy") && layerJson["fadeBy"].is_number()) {
        fadeBy = layerJson["fadeBy"];
      }
      
      ofBlendMode blendMode = OF_BLENDMODE_ALPHA;
      if (layerJson.contains("blendMode") && layerJson["blendMode"].is_string()) {
        blendMode = static_cast<ofBlendMode>(ofBlendModeFromString(layerJson["blendMode"]));
      }
      
      bool useStencil = false;
      if (layerJson.contains("useStencil") && layerJson["useStencil"].is_boolean()) {
        useStencil = layerJson["useStencil"];
      }
      
      int numSamples = 0;
      if (layerJson.contains("numSamples") && layerJson["numSamples"].is_number()) {
        numSamples = layerJson["numSamples"];
      }
      
      bool isDrawn = true;
      if (layerJson.contains("isDrawn") && layerJson["isDrawn"].is_boolean()) {
        isDrawn = layerJson["isDrawn"];
      }
      
      // Create the drawing layer
      synth->addDrawingLayer(name, size, internalFormat, wrap, fadeBy, blendMode, useStencil, numSamples, isDrawn);
      ofLogNotice("SynthConfigSerializer") << "Created drawing layer: " << name;
    }
    return true;
  } catch (const std::exception& e) {
    ofLogError("SynthConfigSerializer") << "Failed to parse drawing layers: " << e.what();
    return false;
  }
}

bool SynthConfigSerializer::parseMods(const nlohmann::json& j, std::shared_ptr<Synth> synth, const ResourceManager& resources) {
  if (!j.contains("mods") || !j["mods"].is_object()) {
    ofLogError("SynthConfigSerializer") << "No mods section in config";
    return false;
  }
  
  try {
    for (const auto& [name, modJson] : j["mods"].items()) {
      if (!modJson.contains("type") || !modJson["type"].is_string()) {
        ofLogError("SynthConfigSerializer") << "Mod '" << name << "' missing type field";
        continue;
      }
      
      std::string type = modJson["type"];
      
      // Parse config map
      ModConfig config;
      if (modJson.contains("config") && modJson["config"].is_object()) {
        for (const auto& [key, value] : modJson["config"].items()) {
          if (value.is_string()) {
            config[key] = value.get<std::string>();
          } else if (value.is_number()) {
            config[key] = std::to_string(value.get<double>());
          } else {
            ofLogWarning("SynthConfigSerializer") << "Mod '" << name << "' config key '" << key << "' has unsupported value type";
          }
        }
      }
      
      // Create Mod using factory
      ModPtr modPtr = ModFactory::create(type, synth, name, std::move(config), resources);
      
      if (!modPtr) {
        ofLogError("SynthConfigSerializer") << "Failed to create Mod '" << name << "' of type '" << type << "'";
        return false;
      }
      
      ofLogNotice("SynthConfigSerializer") << "Created Mod: " << name << " (" << type << ")";
    }
    return true;
  } catch (const std::exception& e) {
    ofLogError("SynthConfigSerializer") << "Failed to parse mods: " << e.what();
    return false;
  }
}

bool SynthConfigSerializer::parseConnections(const nlohmann::json& j, std::shared_ptr<Synth> synth) {
  if (!j.contains("connections") || !j["connections"].is_array()) {
    ofLogNotice("SynthConfigSerializer") << "No connections section in config";
    return true; // Not an error - connections are optional
  }
  
  try {
    // Build a single DSL string from all connections
    std::string connectionsDSL;
    for (const auto& connJson : j["connections"]) {
      if (connJson.is_string()) {
        connectionsDSL += connJson.get<std::string>() + "\n";
      }
    }
    
    // Use existing DSL parser
    if (!connectionsDSL.empty()) {
      synth->addConnections(connectionsDSL);
      ofLogNotice("SynthConfigSerializer") << "Parsed " << j["connections"].size() << " connections";
    }
    
    return true;
  } catch (const std::exception& e) {
    ofLogError("SynthConfigSerializer") << "Failed to parse connections: " << e.what();
    return false;
  }
}

bool SynthConfigSerializer::parseIntents(const nlohmann::json& j, std::shared_ptr<Synth> synth) {
  if (!j.contains("intents") || !j["intents"].is_object()) {
    ofLogNotice("SynthConfigSerializer") << "No intents section in config";
    return true; // Not an error - intents are optional
  }
  
  try {
    std::vector<IntentPtr> intentPresets;
    
    for (const auto& [name, intentJson] : j["intents"].items()) {
      float energy = 0.5f;
      float density = 0.5f;
      float structure = 0.5f;
      float chaos = 0.5f;
      float granularity = 0.5f;
      
      if (intentJson.contains("energy") && intentJson["energy"].is_number()) {
        energy = intentJson["energy"];
      }
      if (intentJson.contains("density") && intentJson["density"].is_number()) {
        density = intentJson["density"];
      }
      if (intentJson.contains("structure") && intentJson["structure"].is_number()) {
        structure = intentJson["structure"];
      }
      if (intentJson.contains("chaos") && intentJson["chaos"].is_number()) {
        chaos = intentJson["chaos"];
      }
      if (intentJson.contains("granularity") && intentJson["granularity"].is_number()) {
        granularity = intentJson["granularity"];
      }
      
      auto intentPtr = Intent::createPreset(name, energy, density, structure, chaos, granularity);
      intentPresets.push_back(intentPtr);
      ofLogNotice("SynthConfigSerializer") << "Created intent: " << name;
    }
    
    // NOTE: Currently Synth manages its own Intent presets in initIntentPresets()
    // We would need to add a method like Synth::setIntentPresets(intentPresets) to use these
    // For now, just log that we parsed them
    ofLogNotice("SynthConfigSerializer") << "Parsed " << intentPresets.size() << " intents (not yet integrated with Synth)";
    
    return true;
  } catch (const std::exception& e) {
    ofLogError("SynthConfigSerializer") << "Failed to parse intents: " << e.what();
    return false;
  }
}

bool SynthConfigSerializer::fromJson(const nlohmann::json& j, std::shared_ptr<Synth> synth, const ResourceManager& resources) {
  if (!synth) {
    ofLogError("SynthConfigSerializer") << "Null Synth pointer";
    return false;
  }
  
  // Validate version
  if (j.contains("version") && j["version"].is_string()) {
    std::string version = j["version"];
    if (version != "1.0") {
      ofLogWarning("SynthConfigSerializer") << "Config version " << version << " may not be compatible (expected 1.0)";
    }
  }
  
  // Log config info
  if (j.contains("name") && j["name"].is_string()) {
    const auto name = j["name"].get<std::string>();
    ofLogNotice("SynthConfigSerializer") << "Loading config: " << name;
    synth->setName(name);
  }
  if (j.contains("description") && j["description"].is_string()) {
    ofLogNotice("SynthConfigSerializer") << "  " << j["description"].get<std::string>();
  }
  
  // Parse each section in order
  if (!parseDrawingLayers(j, synth)) return false;
  if (!parseMods(j, synth, resources)) return false;
  if (!parseConnections(j, synth)) return false;
  if (!parseIntents(j, synth)) return false;
  
  ofLogNotice("SynthConfigSerializer") << "Successfully loaded config";
  return true;
}

bool SynthConfigSerializer::load(std::shared_ptr<Synth> synth, const std::filesystem::path& filepath, const ResourceManager& resources) {
  if (!synth) {
    ofLogError("SynthConfigSerializer") << "Null Synth pointer";
    return false;
  }
  
  if (!std::filesystem::exists(filepath)) {
    ofLogError("SynthConfigSerializer") << "Config file not found: " << filepath;
    return false;
  }
  
  try {
    std::ifstream file(filepath);
    if (!file.is_open()) {
      ofLogError("SynthConfigSerializer") << "Failed to open: " << filepath;
      return false;
    }
    
    nlohmann::json j;
    file >> j;
    file.close();
    
    ofLogNotice("SynthConfigSerializer") << "Parsing config from: " << filepath;
    return fromJson(j, synth, resources);
    
  } catch (const std::exception& e) {
    ofLogError("SynthConfigSerializer") << "Exception loading config: " << e.what();
    return false;
  }
}



} // namespace ofxMarkSynth
