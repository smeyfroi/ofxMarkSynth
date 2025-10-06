//
//  Mod.cpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#include "Mod.hpp"

namespace ofxMarkSynth {



ModPtr findModPtrByName(const std::vector<ModPtr>& mods, const std::string& name) {
  auto it = std::find_if(mods.begin(), mods.end(), [&name](const ModPtr& modPtr) {
//    ofLogNotice() << "Looking for Mod with name " << name << ", checking Mod with name " << modPtr->name;
    return modPtr->name == name;
  });
  if (it == mods.end()) {
    ofLogError() << "No mod found with name " << name;
    ofExit();
  }
  return *it;
}


Mod::Mod(const std::string& name_, const ModConfig&& config_)
: name { name_ },
config { std::move(config_) },
fboPtrs(SINK_FBOPTR_END - SINK_FBOPTR_BEGIN)
{}

bool trySetParameterFromString(ofParameterGroup& group, const std::string& name, const std::string& stringValue) {
  for (const auto& paramPtr : group) {
    if (paramPtr->getName() == name) {
      paramPtr->fromString(stringValue);
      return true;
    }
    if (paramPtr->type() == typeid(ofParameterGroup).name()) {
      return trySetParameterFromString(paramPtr->castGroup(), name, stringValue);
    }
  }
  return false;
}

ofParameterGroup& Mod::getParameterGroup() {
  if (parameters.getName().empty()) {
    parameters.setName(name);
    initParameters();
    for (const auto& [k, v] : config) {
      if (!trySetParameterFromString(parameters, k, v)) ofLogError() << "bad parameter in " << typeid(*this).name() << " with name " << k;
    }
  }
  return parameters;
}

void Mod::connect(int sourceId, ModPtr sinkModPtr, int sinkId) {
  if (! (connections.contains(sourceId) && connections[sourceId] != nullptr)) {
    auto sinksPtr = std::make_unique<Sinks>();
    sinksPtr->emplace_back(std::pair {sinkModPtr, sinkId});
    connections.emplace(sourceId, std::move(sinksPtr));
  } else {
    connections[sourceId]->push_back(std::pair {sinkModPtr, sinkId});
  }
}

//bool Mod::hasSinkFor(int sourceId) {
//  return (connections.contains(sourceId) &&
//          connections[sourceId] != nullptr &&
//          !connections[sourceId]->empty());
//}

template<typename T>
void Mod::emit(int sourceId, const T& value) {
  if (connections[sourceId] == nullptr) return;
  //  if (connections[sourceId] == nullptr) { ofLogError() << "bad connection in " << typeid(*this).name() << " with sourceId " << sourceId; return; }
  std::for_each(connections[sourceId]->begin(),
                connections[sourceId]->end(),
                [&](auto& p) {
    auto& [modPtr, sinkId] = p;
    modPtr->receive(sinkId, value);
  });
}
template void Mod::emit(int sourceId, const glm::vec1& value);
template void Mod::emit(int sourceId, const glm::vec2& value);
template void Mod::emit(int sourceId, const glm::vec3& value);
template void Mod::emit(int sourceId, const glm::vec4& value);
template void Mod::emit(int sourceId, const float& value);
template void Mod::emit(int sourceId, const FboPtr& value);
template void Mod::emit(int sourceId, const ofPixels& value);
template void Mod::emit(int sourceId, const ofFloatPixels& value);
template void Mod::emit(int sourceId, const ofPath& value);
template void Mod::emit(int sourceId, const ofFbo& value);

void Mod::receive(int sinkId, const glm::vec1& point) {
  ofLogError() << "bad receive of glm::vec1 in " << typeid(*this).name();
}

void Mod::receive(int sinkId, const glm::vec2& point) {
  ofLogError() << "bad receive of glm::vec2 in " << typeid(*this).name();
}

void Mod::receive(int sinkId, const glm::vec3& point) {
  ofLogError() << "bad receive of glm::vec3 in " << typeid(*this).name();
}

void Mod::receive(int sinkId, const glm::vec4& point) {
  ofLogError() << "bad receive of glm::vec4 in " << typeid(*this).name();
}

void Mod::receive(int sinkId, const float& point) {
  ofLogError() << "bad receive of float in " << typeid(*this).name();
}

void Mod::receive(int sinkId, const FboPtr& fboPtr_) {
  if (sinkId >= SINK_FBOPTR_BEGIN && sinkId <= SINK_FBOPTR_END) {
    fboPtrs[sinkId - SINK_FBOPTR_BEGIN] = fboPtr_;
//    emit(SOURCE_FBOPTR_BEGIN + sinkId - SINK_FBOPTR_BEGIN, fboPtr_);
  } else {
    ofLogError() << "FboPtr receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void Mod::receive(int sinkId, const ofFloatPixels& pixels) {
  ofLogError() << "bad receive of ofFloatPixels in " << typeid(*this).name();
}

void Mod::receive(int sinkId, const ofPath& path) {
  ofLogError() << "bad receive of ofPath in " << typeid(*this).name();
}

void Mod::receive(int sinkId, const ofFbo& fbo) {
  ofLogError() << "bad receive of ofFbo in " << typeid(*this).name();
}



} // ofxMarkSynth
