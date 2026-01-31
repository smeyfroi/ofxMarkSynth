// ofxMarkSynth

#pragma once

#include "core/Mod.hpp"

#include <string>

namespace ofxMarkSynth {

// Loads performance-scoped preset defaults.
//
// Two preset files are layered during config load:
//
// 1) performanceConfigRootPath/venue-presets.json
// 2) performanceConfigRootPath/mod-params/presets.json
//
// Both files use the same schema:
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
  static std::string getVenuePresetsFilePath();

  // Returns a flattened ModConfig map (paramName -> valueString) for a single file.
  // Missing files/blocks return an empty map.
  static ModConfig loadFromFile(const std::string& filePath, const std::string& modType, const std::string& presetKey);
};

} // namespace ofxMarkSynth
