//
//  Mod.hpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#pragma once

#include "ofxGui.h"

namespace ofxMarkSynth {


class Mod;
using ModConfig = std::unordered_map<std::string, std::string>;
using ModPtr = std::shared_ptr<Mod>;
using ModPtrs = std::vector<ModPtr>;
using SinkId = int;
using Sinks = std::vector<std::pair<ModPtr, SinkId>>;
using SourceId = int;
using Connections = std::unordered_map<SourceId, std::unique_ptr<Sinks>>; // sourceName -> Sinks


class Mod {
  
public:
  Mod(const std::string& name, const ModConfig&& config);
  virtual ~Mod() {};
  virtual void update() = 0;
  virtual void draw() = 0;
  ofParameterGroup& getParameterGroup();
  void addSink(int sourceId, ModPtr sinkModPtr, int sinkId);
  virtual void receive(int sinkId, const glm::vec2& point) {}; // override for sink mods

protected:
  std::string name;
  ModConfig config;
  ofParameterGroup parameters;
  virtual void initParameters() = 0;
  Connections connections;
};


} // ofxMarkSynth
