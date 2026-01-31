// ofxMarkSynth

#include "config/ModPresetLibrary.hpp"
#include "core/Synth.hpp"
#include "ofLog.h"

#include "nlohmann/json.hpp"

#include <filesystem>
#include <fstream>
#include <unordered_map>

namespace ofxMarkSynth {

namespace {

struct FileCache {
  std::filesystem::file_time_type mtime;
  nlohmann::json json;
  bool valid { false };
};

bool jsonValueToString(const nlohmann::json& value, std::string& out) {
  if (value.is_null()) return false;
  if (value.is_string()) {
    out = value.get<std::string>();
    return true;
  }
  if (value.is_number_integer()) {
    out = std::to_string(value.get<int>());
    return true;
  }
  if (value.is_number_float()) {
    out = std::to_string(value.get<double>());
    return true;
  }
  if (value.is_boolean()) {
    out = value.get<bool>() ? "1" : "0";
    return true;
  }
  return false;
}

void mergeOverride(ModConfig& dst, const ModConfig& src) {
  for (const auto& [k, v] : src) {
    dst[k] = v;
  }
}

ModConfig loadPresetBlock(const nlohmann::json& j, const std::string& modType, const std::string& presetKey) {
  if (!j.is_object()) return {};
  if (!j.contains(modType) || !j[modType].is_object()) return {};

  const nlohmann::json& typeObj = j[modType];

  auto readBlock = [&](const std::string& key) -> ModConfig {
    if (!typeObj.contains(key) || !typeObj[key].is_object()) return {};

    ModConfig out;
    for (auto it = typeObj[key].begin(); it != typeObj[key].end(); ++it) {
      const std::string paramName = it.key();
      if (!paramName.empty() && paramName[0] == '_') continue;

      std::string valueStr;
      if (!jsonValueToString(it.value(), valueStr)) {
        continue;
      }
      out[paramName] = valueStr;
    }
    return out;
  };

  ModConfig out;
  mergeOverride(out, readBlock("_default"));
  if (!presetKey.empty() && presetKey != "_default") {
    mergeOverride(out, readBlock(presetKey));
  }
  return out;
}

} // namespace

std::string ModPresetLibrary::getModPresetsFilePath() {
  return Synth::saveConfigFilePath("mod-params/presets.json");
}

std::string ModPresetLibrary::getVenuePresetsFilePath() {
  return Synth::saveConfigFilePath("venue-presets.json");
}

ModConfig ModPresetLibrary::loadFromFile(const std::string& filePath, const std::string& modType, const std::string& presetKey) {
  if (filePath.empty() || modType.empty()) {
    return {};
  }

  static std::unordered_map<std::string, FileCache> cacheByPath;

  std::error_code ec;
  if (!std::filesystem::exists(filePath, ec) || ec) {
    return {};
  }

  const auto mtime = std::filesystem::last_write_time(filePath, ec);
  if (ec) {
    return {};
  }

  auto& cache = cacheByPath[filePath];

  if (!cache.valid || cache.mtime != mtime) {
    try {
      std::ifstream file(filePath);
      if (!file.is_open()) {
        ofLogError("ModPresetLibrary") << "Failed to open presets file: " << filePath;
        cache.valid = false;
        return {};
      }

      nlohmann::json j;
      file >> j;
      file.close();

      cache.json = std::move(j);
      cache.mtime = mtime;
      cache.valid = true;

    } catch (const std::exception& e) {
      ofLogError("ModPresetLibrary") << "Exception loading presets file: " << e.what();
      cache.valid = false;
      return {};
    }
  }

  return loadPresetBlock(cache.json, modType, presetKey);
}

} // namespace ofxMarkSynth
