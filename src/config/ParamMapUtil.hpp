// ParamMapUtil.hpp
// ofxMarkSynth

#pragma once

#include "core/Mod.hpp"
#include "nlohmann/json.hpp"
#include "ofParameter.h"

#include <string>
#include <unordered_map>

namespace ofxMarkSynth {

// A flat map of fully-qualified parameter names to string values.
using ParamMap = std::unordered_map<std::string, std::string>;

struct ParamMapUtil {
  static void serializeParameterGroup(const ofParameterGroup& group, ParamMap& out, const std::string& prefix = "");
  static ParamMap serializeParameterGroup(const ofParameterGroup& group);

  static void deserializeParameterGroup(ofParameterGroup& group, const ParamMap& values, const std::string& prefix = "");

  // Parses a JSON object into a ParamMap.
  // Values are kept as strings (non-strings are dumped).
  static ParamMap parseParamMapJson(const nlohmann::json& j);

  // Writes a ParamMap to a JSON object.
  static nlohmann::json toJson(const ParamMap& m);
};

} // namespace ofxMarkSynth
