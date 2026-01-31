// ParamMapUtil.cpp
// ofxMarkSynth

#include "config/ParamMapUtil.hpp"

namespace ofxMarkSynth {

void ParamMapUtil::serializeParameterGroup(const ofParameterGroup& group, ParamMap& out, const std::string& prefix) {
  for (const auto& param : group) {
    std::string fullName = prefix.empty() ? param->getName() : prefix + "." + param->getName();

    if (param->type() == typeid(ofParameterGroup).name()) {
      serializeParameterGroup(param->castGroup(), out, fullName);
    } else {
      out[fullName] = param->toString();
    }
  }
}

ParamMap ParamMapUtil::serializeParameterGroup(const ofParameterGroup& group) {
  ParamMap out;
  serializeParameterGroup(group, out);
  return out;
}

void ParamMapUtil::deserializeParameterGroup(ofParameterGroup& group, const ParamMap& values, const std::string& prefix) {
  for (auto& param : group) {
    std::string fullName = prefix.empty() ? param->getName() : prefix + "." + param->getName();

    if (param->type() == typeid(ofParameterGroup).name()) {
      deserializeParameterGroup(param->castGroup(), values, fullName);
    } else {
      auto it = values.find(fullName);
      if (it != values.end()) {
        param->fromString(it->second);
      }
    }
  }
}

ParamMap ParamMapUtil::parseParamMapJson(const nlohmann::json& j) {
  ParamMap out;
  if (!j.is_object()) {
    return out;
  }

  for (auto& [k, v] : j.items()) {
    if (v.is_string()) {
      out[k] = v.get<std::string>();
    } else {
      out[k] = v.dump();
    }
  }

  return out;
}

nlohmann::json ParamMapUtil::toJson(const ParamMap& m) {
  nlohmann::json j = nlohmann::json::object();
  for (const auto& [k, v] : m) {
    j[k] = v;
  }
  return j;
}

} // namespace ofxMarkSynth
