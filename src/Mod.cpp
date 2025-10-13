//
//  Mod.cpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#include "Mod.hpp"

namespace ofxMarkSynth {



void assignDrawingLayerPtrToMods(DrawingLayerPtr drawingLayerPtr, std::initializer_list<ModDrawingLayerNamePair> modFboNamePairs) {
  for (const auto& [modPtr, name] : modFboNamePairs) {
      modPtr->receiveDrawingLayerPtr(name, drawingLayerPtr);
  }
}

void connectSourceToSinks(ModPtr sourceModPtr, std::initializer_list<ConnectionsSpec> connectionsSpec) {
  for (const auto& connectionSpec : connectionsSpec) {
    const auto& [sourceId, sinkSpecs] = connectionSpec;
    for (const auto& sinkSpec : sinkSpecs) {
      const auto& [sinkModPtr, sinkId] = sinkSpec;
      sourceModPtr->connect(sourceId, sinkModPtr, sinkId);
    }
  }
}



Mod::Mod(const std::string& name_, const ModConfig&& config_)
: name { name_ },
config { std::move(config_) }
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
template void Mod::emit(int sourceId, const glm::vec2& value);
template void Mod::emit(int sourceId, const glm::vec3& value);
template void Mod::emit(int sourceId, const glm::vec4& value);
template void Mod::emit(int sourceId, const float& value);
template void Mod::emit(int sourceId, const ofFloatPixels& value);
template void Mod::emit(int sourceId, const ofPath& value);
template void Mod::emit(int sourceId, const ofFbo& value);

void Mod::receive(int sinkId, const glm::vec2& point) {
  ofLogError() << "bad receive of glm::vec2 in " << typeid(*this).name();
}

void Mod::receive(int sinkId, const glm::vec3& point) {
  ofLogError() << "bad receive of glm::vec3 in " << typeid(*this).name();
}

void Mod::receive(int sinkId, const glm::vec4& point) {
  ofLogError() << "bad receive of glm::vec4 in " << typeid(*this).name();
}

void Mod::receive(int sinkId, const float& value) {
  ofLogError() << "bad receive of float in " << typeid(*this).name();
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

void Mod::receiveDrawingLayerPtr(const std::string& name, const DrawingLayerPtr drawingLayerPtr) {
  auto& drawingLayerPtrs = namedDrawingLayerPtrs[name];
  drawingLayerPtrs.push_back(drawingLayerPtr);
  currentDrawingLayerIndices[name] = 0;
}

std::optional<DrawingLayerPtr> Mod::getNamedDrawingLayerPtr(const std::string& name, size_t index) {
  if (!namedDrawingLayerPtrs.contains(name)) return std::nullopt;
  auto& drawingLayerPtrs = namedDrawingLayerPtrs[name];
  if (index >= drawingLayerPtrs.size()) return std::nullopt;
  return drawingLayerPtrs[index];
}

std::optional<DrawingLayerPtr> Mod::getCurrentNamedDrawingLayerPtr(const std::string& name) {
  auto index = currentDrawingLayerIndices[name];
  return getNamedDrawingLayerPtr(name, index);
}



} // ofxMarkSynth
