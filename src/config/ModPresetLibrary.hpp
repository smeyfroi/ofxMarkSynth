// ofxMarkSynth

#pragma once

#include "core/Mod.hpp"

#include "nlohmann/json.hpp"

#include <string>

namespace ofxMarkSynth {

// Loads preset defaults used to initialize Mod parameters.
//
// Preset defaults are layered during config load:
//
// 1) Session-scoped presets embedded in `session-config.json` under key `modPresets` (passed via ResourceManager)
// 2) performanceConfigRootPath/mod-params/presets.json
//
// Both sources use the same schema:
//
// {
//   "VideoFlowSource": {
//     "_default": { "MinSpeedMagnitude": "0.4" },
//     "CameraWide": { "MinSpeedMagnitude": "0.35" }
//   }
// }
//
// The effective defaults for a Mod are computed per (type, presetKey):
// - Apply [type]["_default"]
// - Apply [type][presetKey] (if present)
//
// Presets are applied before default capture in Mod::getParameterGroup().
class ModPresetLibrary {
public:
  static std::string getModPresetsFilePath();

  // Returns a flattened ModConfig map (paramName -> valueString) for a JSON object with the preset schema.
  // Missing blocks return an empty map.
  static ModConfig loadFromJson(const nlohmann::json& j, const std::string& modType, const std::string& presetKey);

  // Returns a flattened ModConfig map (paramName -> valueString) for a single file.
  // Missing files/blocks return an empty map.
  static ModConfig loadFromFile(const std::string& filePath, const std::string& modType, const std::string& presetKey);
};

} // namespace ofxMarkSynth
