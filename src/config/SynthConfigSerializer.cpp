//
//  SynthConfigSerializer.cpp
//  ofxMarkSynth
//
//  Created for config serialization system
//

#include "config/SynthConfigSerializer.hpp"
#include <algorithm>
#include <cmath>
#include "core/Synth.hpp"
#include "core/Intent.hpp"
#include "TimeStringUtil.h"
#include "sourceMods/AudioDataSourceMod.hpp"
#include "config/ModPresetLibrary.hpp"
#include "ofLog.h"
#include "ofUtils.h"
#include <fstream>
#include <filesystem>
#include <sstream>



namespace ofxMarkSynth {

using OrderedJson = nlohmann::ordered_json;

namespace {

// JSON extraction helpers - reduce repetitive contains/is_type/get patterns
template<typename T>
T getJsonValue(const nlohmann::json& j, const std::string& key, T defaultValue) {
    if (j.contains(key) && !j[key].is_null()) {
        return j[key].get<T>();
    }
    return defaultValue;
}

// Specialization for bool (JSON stores as boolean type)
bool getJsonBool(const nlohmann::json& j, const std::string& key, bool defaultValue) {
    if (j.contains(key) && j[key].is_boolean()) {
        return j[key].get<bool>();
    }
    return defaultValue;
}

// Specialization for float (JSON stores as number)
float getJsonFloat(const nlohmann::json& j, const std::string& key, float defaultValue) {
    if (j.contains(key) && j[key].is_number()) {
        return j[key].get<float>();
    }
    return defaultValue;
}

// Specialization for int (JSON stores as number)
int getJsonInt(const nlohmann::json& j, const std::string& key, int defaultValue) {
    if (j.contains(key) && j[key].is_number()) {
        return j[key].get<int>();
    }
    return defaultValue;
}

// Specialization for string
std::string getJsonString(const nlohmann::json& j, const std::string& key, const std::string& defaultValue = "") {
    if (j.contains(key) && j[key].is_string()) {
        return j[key].get<std::string>();
    }
    return defaultValue;
}

} // anonymous namespace

int SynthConfigSerializer::glEnumFromString(const std::string& str) {
  // Common GL enums used in ofxMarkSynth
  if (str == "GL_RGBA") return GL_RGBA;
  if (str == "GL_RGB") return GL_RGB;
  if (str == "GL_RGBA32F") return GL_RGBA32F;
  if (str == "GL_RGB32F") return GL_RGB32F;
  if (str == "GL_RG32F") return GL_RG32F;
  if (str == "GL_RGBA16F") return GL_RGBA16F;
  if (str == "GL_RGB16F") return GL_RGB16F;
  if (str == "GL_RG16F") return GL_RG16F;
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

SynthConfigSerializer::NamedLayers SynthConfigSerializer::parseDrawingLayers(const OrderedJson& j, std::shared_ptr<Synth> synth) {
  if (!j.contains("drawingLayers") || !j["drawingLayers"].is_object()) {
    ofLogNotice("SynthConfigSerializer") << "No drawingLayers section in config";
    return {};
  }
  
  SynthConfigSerializer::NamedLayers layers;
  try {
    for (const auto& [name, layerJson] : j["drawingLayers"].items()) {
      // Parse size (special case - array)
      glm::vec2 size { 1080, 1080 };
      if (layerJson.contains("size") && layerJson["size"].is_array() && layerJson["size"].size() == 2) {
        size.x = layerJson["size"][0];
        size.y = layerJson["size"][1];
      }
      
      // Parse layer properties using helpers
      std::string formatStr = getJsonString(layerJson, "internalFormat");
      GLint internalFormat = formatStr.empty() ? GL_RGBA : glEnumFromString(formatStr);
      
      std::string wrapStr = getJsonString(layerJson, "wrap");
      int wrap = wrapStr.empty() ? GL_CLAMP_TO_EDGE : glEnumFromString(wrapStr);
      
      std::string blendStr = getJsonString(layerJson, "blendMode");
      ofBlendMode blendMode = blendStr.empty() ? OF_BLENDMODE_ALPHA : static_cast<ofBlendMode>(ofBlendModeFromString(blendStr));
      
      bool clearOnUpdate = getJsonBool(layerJson, "clearOnUpdate", false);
      bool useStencil = getJsonBool(layerJson, "useStencil", false);
      int numSamples = getJsonInt(layerJson, "numSamples", 0);
      bool isDrawn = getJsonBool(layerJson, "isDrawn", true);
      bool isOverlay = getJsonBool(layerJson, "isOverlay", false);
      float alpha = getJsonFloat(layerJson, "alpha", 1.0f);
      bool paused = getJsonBool(layerJson, "paused", false);
      std::string description = getJsonString(layerJson, "description");
      
      // Set layer controller initial values
      synth->layerController->setInitialAlpha(name, alpha);
      synth->layerController->setInitialPaused(name, paused);
      
      // Create layer
      auto layerPtr = synth->addDrawingLayer(name, size, internalFormat, wrap, clearOnUpdate, blendMode, useStencil, numSamples, isDrawn, isOverlay, description);
      layers[name] = layerPtr;
      ofLogNotice("SynthConfigSerializer") << "Created drawing layer: " << name << " (size: " << size.x << "x" << size.y << ", format: " << internalFormat << ")";
    }
    return layers;
  } catch (const std::exception& e) {
    ofLogError("SynthConfigSerializer") << "Failed to parse drawing layers: " << e.what();
    return layers;
  }
}

bool SynthConfigSerializer::parseMods(const OrderedJson& j, std::shared_ptr<Synth> synth, const ResourceManager& resources, const NamedLayers& layers) {
  if (!j.contains("mods") || !j["mods"].is_object()) {
    ofLogError("SynthConfigSerializer") << "No mods section in config";
    return false;
  }
  
  try {
    for (const auto& [name, modJson] : j["mods"].items()) {
      // Convention: underscore-prefixed keys are comments/metadata.
      if (!name.empty() && name[0] == '_') {
        continue;
      }

      if (!modJson.is_object()) {
        ofLogWarning("SynthConfigSerializer") << "Mod entry '" << name << "' is not an object, skipping";
        continue;
      }

      if (!modJson.contains("type") || !modJson["type"].is_string()) {
        ofLogError("SynthConfigSerializer") << "Mod '" << name << "' missing type field";
        continue;
      }

      std::string type = modJson["type"];

      std::string presetName;
      if (modJson.contains("preset") && modJson["preset"].is_string()) {
        presetName = modJson["preset"].get<std::string>();
      }

      const std::string presetKey = presetName.empty() ? "_default" : presetName;

      // Parse config map
      ModConfig config;
      if (modJson.contains("config") && modJson["config"].is_object()) {
        for (const auto& [key, value] : modJson["config"].items()) {
          if (!key.empty() && key[0] == '_') {
            continue;
          }
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

      // For UI/debugging (e.g. node editor): store the explicit preset name from config.
      modPtr->setPresetName(presetName);

      // Performance-scoped defaults (applied before capturing Mod defaults):
      // - venue-presets.json
      // - mod-params/presets.json
      ModConfig presetDefaults;
      for (const auto& [k, v] : ModPresetLibrary::loadFromFile(ModPresetLibrary::getVenuePresetsFilePath(), type, presetKey)) {
        presetDefaults[k] = v;
      }
      for (const auto& [k, v] : ModPresetLibrary::loadFromFile(ModPresetLibrary::getModPresetsFilePath(), type, presetKey)) {
        presetDefaults[k] = v;
      }
      modPtr->setPresetConfig(std::move(presetDefaults));

      ofLogNotice("SynthConfigSerializer") << "Created Mod: " << name << " (" << type << ")";
      
      if (modJson.contains("layers") && modJson["layers"].is_object()) {
        for (const auto& [layerPtrName, value] : modJson["layers"].items()) {
          if (value.is_array()) {
            for (const std::string layerName : value) {
              auto it = layers.find(layerName);
              if (it == layers.end()) {
                ofLogWarning("SynthConfigSerializer") << "Mod '" << layerName << "' references unknown drawing layer '" << layerName << "'";
                continue;
              }
              auto drawingLayerPtr = it->second;
              modPtr->receiveDrawingLayerPtr(layerPtrName, drawingLayerPtr);
              ofLogNotice("SynthConfigSerializer") << "  Assigned drawing layer '" << layerName << "' to Mod '" << name << "' layer key '" << layerPtrName << "'";
            }
          } else {
            ofLogError("SynthConfigSerializer") << "Mod '" << name << "' layers key '" << layerPtrName << "' is not an array";
          }
        }
      }
    }
    return true;
  } catch (const std::exception& e) {
    ofLogError("SynthConfigSerializer") << "Failed to parse mods: " << e.what();
    return false;
  }
}

bool SynthConfigSerializer::parseConnections(const OrderedJson& j, std::shared_ptr<Synth> synth) {
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

static ofFloatColor parseFloatColor(const std::string& str) {
  std::vector<float> values;
  std::stringstream ss(str);
  std::string token;
  while (std::getline(ss, token, ',')) {
    values.push_back(std::stof(ofTrim(token)));
  }
  if (values.size() >= 4) {
    return ofFloatColor(values[0], values[1], values[2], values[3]);
  } else if (values.size() >= 3) {
    return ofFloatColor(values[0], values[1], values[2], 1.0f);
  }
  return ofFloatColor(0, 0, 0, 1);
}

bool SynthConfigSerializer::parseSynthConfig(const OrderedJson& j, std::shared_ptr<Synth> synth) {
  if (!j.contains("synth") || !j["synth"].is_object()) {
    return true;
  }
  
  const auto& s = j["synth"];
  
  // Agency
  if (s.contains("agency") && s["agency"].is_number()) {
    float agency = s["agency"].get<float>();
    synth->setAgency(agency);
    ofLogNotice("SynthConfigSerializer") << "  Synth agency: " << agency;
  }
  
  // Background color
  std::string bgColorStr = getJsonString(s, "backgroundColor");
  if (!bgColorStr.empty()) {
    synth->backgroundColorParameter.set(parseFloatColor(bgColorStr));
    ofLogNotice("SynthConfigSerializer") << "  Synth backgroundColor: " << bgColorStr;
  }
  
  // Background brightness (was "backgroundMultiplier" in older configs)
  if (s.contains("backgroundBrightness") && s["backgroundBrightness"].is_number()) {
    float brightness = s["backgroundBrightness"].get<float>();
    synth->backgroundBrightnessParameter.set(brightness);
    ofLogNotice("SynthConfigSerializer") << "  Synth backgroundBrightness: " << brightness;
  } else if (s.contains("backgroundMultiplier") && s["backgroundMultiplier"].is_number()) {
    // Legacy mapping: old multiplier acted on RGB directly.
    // New behavior: treat this as a target brightness for the tinted background.
    // Empirical conversion: 0.1 -> 0.035.
    constexpr float MULTIPLIER_TO_BRIGHTNESS = 0.35f;
    float mult = s["backgroundMultiplier"].get<float>();
    float brightness = std::clamp(mult * MULTIPLIER_TO_BRIGHTNESS, 0.0f, 1.0f);
    synth->backgroundBrightnessParameter.set(brightness);
    ofLogNotice("SynthConfigSerializer") << "  Synth backgroundBrightness (from legacy multiplier): " << brightness;
  }
  
  // Manual bias decay
  if (s.contains("manualBiasDecaySec") && s["manualBiasDecaySec"].is_number()) {
    float decay = s["manualBiasDecaySec"].get<float>();
    synth->manualBiasDecaySecParameter.set(decay);
    ofLogNotice("SynthConfigSerializer") << "  Manual bias decay time: " << decay;
  }
  
  // Base manual bias
  if (s.contains("baseManualBias") && s["baseManualBias"].is_number()) {
    float bias = s["baseManualBias"].get<float>();
    synth->baseManualBiasParameter.set(bias);
    ofLogNotice("SynthConfigSerializer") << "  Base manual bias: " << bias;
  }
  
  // Crossfade delay
  if (s.contains("crossfadeDelaySec") && s["crossfadeDelaySec"].is_number()) {
    float delaySec = s["crossfadeDelaySec"].get<float>();
    synth->configTransitionManager->getDelaySecParameter().set(delaySec);
    ofLogNotice("SynthConfigSerializer") << "  Crossfade delay sec: " << delaySec;
  }

  // Crossfade duration
  if (s.contains("crossfadeDuration") && s["crossfadeDuration"].is_number()) {
    float duration = s["crossfadeDuration"].get<float>();
    synth->configTransitionManager->getDurationParameter().set(duration);
    ofLogNotice("SynthConfigSerializer") << "  Crossfade duration: " << duration;
  }
  
  return true;
}

bool SynthConfigSerializer::parseIntents(const OrderedJson& j, std::shared_ptr<Synth> synth) {
  if (!j.contains("intents") || !j["intents"].is_array()) {
    ofLogNotice("SynthConfigSerializer") << "No intents array in config";
    return true;
  }
  
  try {
    std::vector<IntentPtr> intentPresets;
    
    for (const auto& intentJson : j["intents"]) {
      std::string name = getJsonString(intentJson, "name");
      if (name.empty()) {
        ofLogWarning("SynthConfigSerializer") << "Intent missing 'name' field, skipping";
        continue;
      }
      
      float energy = getJsonFloat(intentJson, "energy", 0.5f);
      float density = getJsonFloat(intentJson, "density", 0.5f);
      float structure = getJsonFloat(intentJson, "structure", 0.5f);
      float chaos = getJsonFloat(intentJson, "chaos", 0.5f);
      float granularity = getJsonFloat(intentJson, "granularity", 0.5f);
      
      auto intentPtr = Intent::createPreset(name, energy, density, structure, chaos, granularity);

      // Optional per-intent UI metadata for tooltips (safe to omit in configs).
      if (intentJson.contains("ui") && intentJson["ui"].is_object()) {
        const auto& ui = intentJson["ui"];
        if (ui.contains("notes") && ui["notes"].is_string()) {
          intentPtr->setUiNotes(ui["notes"].get<std::string>());
        }
        if (ui.contains("impact") && ui["impact"].is_object()) {
          Intent::UiImpact impact;
          for (auto& item : ui["impact"].items()) {
            const std::string& key = item.key();
            const auto& value = item.value();
            if (!value.is_number()) continue;
            int v = value.is_number_integer() ? value.get<int>() : static_cast<int>(std::lround(value.get<float>()));
            v = std::clamp(v, -3, 3);
            impact.emplace_back(key, v);
          }
          if (!impact.empty()) {
            intentPtr->setUiImpact(impact);
          }
        }
      }
      intentPresets.push_back(intentPtr);
      ofLogNotice("SynthConfigSerializer") << "Created intent: " << name;
    }
    
    if (!intentPresets.empty()) {
      synth->setIntentPresets(intentPresets);
    }
    
    return true;
  } catch (const std::exception& e) {
    ofLogError("SynthConfigSerializer") << "Failed to parse intents: " << e.what();
    return false;
  }
}

bool SynthConfigSerializer::fromJson(const OrderedJson& j, std::shared_ptr<Synth> synth, const ResourceManager& resources, const std::string& configId) {
  if (!synth) {
    ofLogError("SynthConfigSerializer") << "Null Synth pointer";
    return false;
  }
  
  // Validate version
  if (j.contains("version") && j["version"].is_string()) {
    std::string version = j["version"];
    // Backward-compatible: accept 1.0 and 1.1 (Improvisation1 configs are now 1.1).
    if (version != "1.1" && version != "1.0") {
      ofLogWarning("SynthConfigSerializer") << "Config version " << version
          << " may not be compatible (expected 1.1 or 1.0)";
    }
  }

  ofLogNotice("SynthConfigSerializer") << "Loading config: " << configId;
  
  if (j.contains("description") && j["description"].is_string()) {
    ofLogNotice("SynthConfigSerializer") << "  " << j["description"].get<std::string>();
  }
  
  // Parse optional duration field for performance timing
  if (j.contains("duration") && j["duration"].is_string()) {
    int durationSec = parseTimeStringToSeconds(j["duration"].get<std::string>());
    synth->getPerformanceNavigator().setConfigDurationSec(durationSec);
    ofLogNotice("SynthConfigSerializer") << "  Config duration: " << j["duration"].get<std::string>();
  } else {
    synth->getPerformanceNavigator().setConfigDurationSec(0);
  }

  // Parse performer cues (optional).
  // Missing keys are treated as false, so configs can show audio-only or video-only cues.
  bool performerCueAudio = false;
  bool performerCueVideo = false;
  if (j.contains("performerCues") && j["performerCues"].is_object()) {
    const auto& cues = j["performerCues"];
    if (cues.contains("audio") && cues["audio"].is_boolean()) {
      performerCueAudio = cues["audio"].get<bool>();
    }
    if (cues.contains("video") && cues["video"].is_boolean()) {
      performerCueVideo = cues["video"].get<bool>();
    }
  }
  synth->setPerformerCues(performerCueAudio, performerCueVideo);

  // Parse synth-level configuration (agency, backgroundColor, backgroundBrightness)
  parseSynthConfig(j, synth);

  // Parse each section in order
  SynthConfigSerializer::NamedLayers namedLayers = parseDrawingLayers(j, synth);
  parseMods(j, synth, resources, namedLayers);
  parseConnections(j, synth);
  parseIntents(j, synth);
  
  // Apply initial intent configuration (must be after parseIntents creates the activations)
  if (j.contains("synth") && j["synth"].contains("initialIntent")) {
    const auto& intentConfig = j["synth"]["initialIntent"];
    
    if (intentConfig.contains("strength") && intentConfig["strength"].is_number()) {
      synth->setIntentStrength(intentConfig["strength"]);
      ofLogNotice("SynthConfigSerializer") << "Set intent strength: " << intentConfig["strength"].get<float>();
    }
    
    if (intentConfig.contains("activations") && intentConfig["activations"].is_array()) {
      const auto& activations = intentConfig["activations"];
      size_t intentCount = synth->getIntentCount();
      
      for (size_t i = 0; i < activations.size(); ++i) {
        if (i >= intentCount) {
          ofLogWarning("SynthConfigSerializer") << "intent.activations[" << i
              << "] ignored: only " << intentCount << " intents defined";
          break;
        }
        if (activations[i].is_number()) {
          synth->setIntentActivation(i, activations[i]);
          ofLogNotice("SynthConfigSerializer") << "Set intent[" << i << "] activation: " << activations[i].get<float>();
        }
      }
    }
  }
  
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
  
  // Extract config id from filename stem
  std::string configId = filepath.stem().string();
  
  try {
    std::ifstream file(filepath);
    if (!file.is_open()) {
      ofLogError("SynthConfigSerializer") << "Failed to open: " << filepath;
      return false;
    }
    
    nlohmann::ordered_json j;
    file >> j;
    file.close();
    
    ofLogNotice("SynthConfigSerializer") << "Parsing config from: " << filepath;
    return fromJson(j, synth, resources, configId);
    
  } catch (const std::exception& e) {
    ofLogError("SynthConfigSerializer") << "Exception loading config: " << e.what();
    return false;
  }
}



} // namespace ofxMarkSynth
