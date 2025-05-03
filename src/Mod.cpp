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
config { config_ }
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


} // ofxMarkSynth
