//
//  Mod.cpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#include "Mod.hpp"

namespace ofxMarkSynth {


Mod::Mod(const std::string& name_, const ModConfig&& config_)
: name { name_ },
config { std::move(config_) }
{}

ofParameterGroup& Mod::getParameterGroup() {
  if (parameters.getName().empty()) {
    parameters.setName(name);
    initParameters();
    for (const auto& [k, v] : config) {
      if (!parameters.contains(k)) {
        ofLogError() << "bad parameter in " << typeid(*this).name() << " with name " << k;
      } else {
        parameters.get(k).fromString(v);
      }
    }
  }
  return parameters;
}

void Mod::addSink(int sourceId, ModPtr sinkModPtr, int sinkId) {
  if (!connections.contains(sourceId)) {
    auto sinksPtr = std::make_unique<Sinks>();
    sinksPtr->emplace_back(std::pair {sinkModPtr, sinkId});
    connections.emplace(sourceId, std::move(sinksPtr));
  } else {
    connections[sourceId]->push_back(std::pair {sinkModPtr, sinkId});
  }
}

bool Mod::hasSinkFor(int sourceId) {
  return (connections.contains(sourceId) && connections[sourceId] != nullptr && !connections[sourceId]->empty());
}

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

void Mod::receive(int sinkId, const FboPtr& fboPtr) {
  ofLogError() << "bad receive of FboPtr in " << typeid(*this).name();
}


} // ofxMarkSynth
