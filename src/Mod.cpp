//
//  Mod.cpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#include "Mod.hpp"
#include "Synth.hpp"



namespace ofxMarkSynth {



int Mod::nextId = 1000;

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



Mod::Mod(std::shared_ptr<Synth> synthPtr_, const std::string& name_, ModConfig config_)
: synthPtr { std::move(synthPtr_) },
name { name_ },
id { nextId },
config { std::move(config_) }
{
  nextId += 1000;
}

int Mod::getId() const {
  return id;
}

const std::string& Mod::getName() const {
  return name;
}

std::optional<std::reference_wrapper<ofAbstractParameter>> findParameterByNamePrefix(ofParameterGroup& group, const std::string& namePrefix) {
  for (const auto& paramPtr : group) {
    if (paramPtr->getName().rfind(namePrefix, 0) == 0) {
      return std::ref(*paramPtr);
    }
    if (paramPtr->type() == typeid(ofParameterGroup).name()) {
      if (auto found = findParameterByNamePrefix(paramPtr->castGroup(), namePrefix)) {
        return found;
      }
    }
  }
  return std::nullopt;
}

std::optional<std::reference_wrapper<ofAbstractParameter>> Mod::findParameterByNamePrefix(const std::string& name) {
  return ::ofxMarkSynth::findParameterByNamePrefix(getParameterGroup(), name);
}

bool trySetParameterFromString(ofParameterGroup& group, const std::string& name, const std::string& stringValue) {
  if (auto paramOpt = findParameterByNamePrefix(group, name)) {
    paramOpt->get().fromString(stringValue);
    return true;
  }
  return false;
}

using ParamValueMap = Mod::ParamValueMap;

static void serializeParameterGroup(const ofParameterGroup& group, ParamValueMap& out, const std::string& prefix = "") {
  for (const auto& param : group) {
    std::string fullName = prefix.empty() ? param->getName() : prefix + "." + param->getName();

    if (param->type() == typeid(ofParameterGroup).name()) {
      serializeParameterGroup(param->castGroup(), out, fullName);
    } else {
      out[fullName] = param->toString();
    }
  }
}

static ParamValueMap serializeParameterGroup(const ofParameterGroup& group) {
  ParamValueMap out;
  serializeParameterGroup(group, out);
  return out;
}

ofParameterGroup& Mod::getParameterGroup() {
  if (parameters.getName().empty()) {
    parameters.setName(name);
    initParameters();

    if (!defaultParameterValuesCaptured) {
      defaultParameterValues = serializeParameterGroup(parameters);
      defaultParameterValuesCaptured = true;
    }

    for (const auto& [k, v] : config) {
      if (!k.empty() && k[0] == '_') {
        continue;
      }
      if (!trySetParameterFromString(parameters, k, v)) {
        ofLogError("Mod") << "Bad parameter: " << k << " not one of: " << parameters.toString();
      }
    }

    // Sync all registered ParamControllers with their current parameter values,
    // possibly loaded from config after the controller was created.
    for (auto& [name, controllerPtr] : sourceNameControllerPtrMap) {
      if (controllerPtr) {
        controllerPtr->syncWithParameter();
      }
    }
  }
  return parameters;
}

ParamValueMap Mod::getCurrentParameterValues() {
  return serializeParameterGroup(getParameterGroup());
}

const ParamValueMap& Mod::getDefaultParameterValues() {
  getParameterGroup(); // Ensure init has run.
  return defaultParameterValues;
}

void Mod::connect(int sourceId, ModPtr sinkModPtr, int sinkId) {
  assert(sinkModPtr != nullptr);
  if (! (connections.contains(sourceId) && connections[sourceId] != nullptr)) {
    auto sinksPtr = std::make_unique<Sinks>();
    sinksPtr->emplace_back(std::pair {sinkModPtr, sinkId});
    connections.emplace(sourceId, std::move(sinksPtr));
  } else {
    connections[sourceId]->push_back(std::pair {sinkModPtr, sinkId});
  }
}

void Mod::registerControllerForSource(const std::string& sourceName, BaseParamController& controller) {
  if (sourceName.empty()) {
    ofLogError("Mod") << "Empty source name when registering controller in Mod '" << name << "'";
    return;
  }
  if (sourceNameControllerPtrMap.contains(sourceName)) {
    ofLogWarning("Mod") << "Controller already registered for source '" << sourceName
                         << "' in Mod '" << name << "', overwriting";
  }
  sourceNameControllerPtrMap[sourceName] = &controller;
}

std::shared_ptr<Synth> Mod::getSynth() const {
  auto synth = synthPtr.lock();
  if (!synth) {
    ofLogError("Mod") << "Synth expired for Mod '" << name << "'";
  }
  return synth;
}

float Mod::getAgency() const {
  if (auto synth = getSynth()) {
    return synth->getAgency();
  }
  return 0.0f;
}

int Mod::getSourceId(const std::string& sourceName) {
  if (!sourceNameIdMap.contains(sourceName)) {
    std::vector<std::string> keys;
    for (const auto& [key, _] : sourceNameIdMap) {
      keys.push_back(key);
    }
    ofLogError("Mod") << "Bad source name: " << sourceName << " not one of: " << ofJoinString(keys, ", ");
  }
  return sourceNameIdMap.at(sourceName);
}

int Mod::getSinkId(const std::string& sinkName) {
  if (!sinkNameIdMap.contains(sinkName)) {
    std::vector<std::string> keys;
    for (const auto& [key, _] : sinkNameIdMap) {
      keys.push_back(key);
    }
    ofLogError("Mod") << "Bad sink name: " << sinkName << " not one of: " << ofJoinString(keys, ", ");
  }
  return sinkNameIdMap.at(sinkName);
}

template<typename T>
void Mod::emit(int sourceId, const T& value) {
  if (!connections.contains(sourceId)) return;
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
template void Mod::emit(int sourceId, const ofTexture& value);
template void Mod::emit(int sourceId, const std::string& value);

void Mod::receive(int sinkId, const glm::vec2& point) {
  ofLogError("Mod") << name << " (" << typeid(*this).name() << ") bad receive of glm::vec2";
}

void Mod::receive(int sinkId, const glm::vec3& point) {
  ofLogError("Mod") << name << " (" << typeid(*this).name() << ") bad receive of glm::vec3";
}

void Mod::receive(int sinkId, const glm::vec4& point) {
  ofLogError("Mod") << name << " (" << typeid(*this).name() << ") bad receive of glm::vec4";
}

void Mod::receive(int sinkId, const float& value) {
  ofLogError("Mod") << name << " (" << typeid(*this).name() << ") float receive for unknown sinkId " << sinkId;
}

void Mod::receive(int sinkId, const ofFloatPixels& pixels) {
  ofLogError("Mod") << name << " (" << typeid(*this).name() << ") bad receive of ofFloatPixels";
}

void Mod::receive(int sinkId, const ofPath& path) {
  ofLogError("Mod") << name << " (" << typeid(*this).name() << ") bad receive of ofPath";
}

void Mod::receive(int sinkId, const ofFbo& fbo) {
  ofLogError("Mod") << name << " (" << typeid(*this).name() << ") bad receive of ofFbo";
}

void Mod::receive(int sinkId, const ofTexture& texture) {
  ofLogError("Mod") << name << " (" << typeid(*this).name() << ") bad receive of ofTexture";
}

void Mod::receive(int sinkId, const std::string& text) {
  ofLogWarning("Mod") << name << " (" << typeid(*this).name() << ") received string but doesn't handle it";
}

void Mod::receiveDrawingLayerPtr(const std::string& name, const DrawingLayerPtr drawingLayerPtr) {
  auto& drawingLayerPtrs = namedDrawingLayerPtrs[name];
  drawingLayerPtrs.push_back(drawingLayerPtr);
}

void Mod::syncControllerAgencies() {
  float a = getAgency();
  for (auto& kv : sourceNameControllerPtrMap) {
    if (kv.second) kv.second->setAgency(a);
  }
}

std::optional<DrawingLayerPtr> Mod::getNamedDrawingLayerPtr(const std::string& name, int index) {
  if (index < 0) return std::nullopt;
  if (!namedDrawingLayerPtrs.contains(name)) return std::nullopt;
  auto& drawingLayerPtrs = namedDrawingLayerPtrs[name];
  if (index >= static_cast<int>(drawingLayerPtrs.size())) return std::nullopt;
  auto layerPtr = drawingLayerPtrs[index];
  if (layerPtr->pauseState == DrawingLayer::PauseState::PAUSED) return std::nullopt;
  return layerPtr;
}

std::optional<DrawingLayerPtr> Mod::getCurrentNamedDrawingLayerPtr(const std::string& name) {
  auto index = currentDrawingLayerIndices[name];
  return getNamedDrawingLayerPtr(name, index);
}

std::optional<std::string> Mod::getRandomLayerName() {
  if (namedDrawingLayerPtrs.empty()) return std::nullopt;
  auto it = namedDrawingLayerPtrs.begin();
  std::advance(it, (size_t)ofRandom(0, namedDrawingLayerPtrs.size()));
  return it->first;
}

void Mod::changeDrawingLayer() {
  if (const auto& layerNameOpt = getRandomLayerName()) changeDrawingLayer(*layerNameOpt);
}

// Return to the default layer 0 between drawing on other layers
void Mod::changeDrawingLayer(const std::string& layerName) {
  if (currentDrawingLayerIndices[layerName] == 0) {
    int newIndex = ofRandom(0, namedDrawingLayerPtrs[layerName].size());
    if (newIndex == 0) newIndex = -1;
    currentDrawingLayerIndices[layerName] = newIndex;
  } else {
    currentDrawingLayerIndices[layerName] = 0;
  }
  ofLogNotice("Mod") << "'" << name << "' changing current drawing layer '" << layerName << "' to index " << currentDrawingLayerIndices[layerName] << " : " << ((currentDrawingLayerIndices[layerName] >= 0) ? namedDrawingLayerPtrs[layerName][currentDrawingLayerIndices[layerName]]->name : "NONE");
}

// Return to the default layer 0
void Mod::resetDrawingLayer() {
  if (const auto& layerNameOpt = getRandomLayerName()) resetDrawingLayer(*layerNameOpt);
}

void Mod::resetDrawingLayer(const std::string& layerName) {
  currentDrawingLayerIndices[layerName] = 0;
  ofLogNotice("Mod") << "'" << name << "' reset current drawing layer '" << layerName << "'";
}

// Disable drawing for a lyer
void Mod::disableDrawingLayer() {
  if (const auto& layerNameOpt = getRandomLayerName()) disableDrawingLayer(*layerNameOpt);
}

void Mod::disableDrawingLayer(const std::string& layerName) {
  currentDrawingLayerIndices[layerName] = -1;
  ofLogNotice("Mod") << "'" << name << "' disable current drawing layer '" << layerName << "'";
}



} // ofxMarkSynth
