//
//  Mod.hpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#pragma once

#include "ofxGui.h"
#include "PingPongFbo.h"


namespace ofxMarkSynth {


class Mod;
using ModConfig = std::unordered_map<std::string, std::string>;
using ModPtr = std::shared_ptr<Mod>;
using ModPtrs = std::vector<ModPtr>;

using SinkId = int;
using Sinks = std::vector<std::pair<ModPtr, SinkId>>;
using SourceId = int;
using Connections = std::unordered_map<SourceId, std::unique_ptr<Sinks>>;

using FboPtr = std::shared_ptr<PingPongFbo>;
using FboPtrs = std::vector<FboPtr>;


class Mod {
  
public:
  Mod(const std::string& name, const ModConfig&& config);
  virtual ~Mod() {};
  virtual void update() {};
  virtual void draw() {};
  virtual bool keyPressed(int key) { return false; };
  ofParameterGroup& getParameterGroup();
  void addSink(int sourceId, ModPtr sinkModPtr, int sinkId);
  bool hasSinkFor(int sourceId);
  virtual void receive(int sinkId, const glm::vec1& point);
  virtual void receive(int sinkId, const glm::vec2& point);
  virtual void receive(int sinkId, const glm::vec3& point);
  virtual void receive(int sinkId, const glm::vec4& point);
  virtual void receive(int sinkId, const float& value);
  virtual void receive(int sinkId, const FboPtr& fboPtr);
  virtual void receive(int sinkId, const ofPixels& pixels);

  static constexpr int SOURCE_FBO_BEGIN = -100;
  static constexpr int SOURCE_FBO = SOURCE_FBO_BEGIN;
  static constexpr int SOURCE_FBO_2 = SOURCE_FBO + 1;
  static constexpr int SOURCE_FBO_3 = SOURCE_FBO_2 + 1;
  static constexpr int SOURCE_FBO_4 = SOURCE_FBO_3 + 1;
  static constexpr int SOURCE_FBO_END = SOURCE_FBO_4;
  
  static constexpr int SINK_FBO_BEGIN = -200;
  static constexpr int SINK_FBO = SINK_FBO_BEGIN;
  static constexpr int SINK_FBO_2 = SINK_FBO + 1;
  static constexpr int SINK_FBO_3 = SINK_FBO_2 + 1;
  static constexpr int SINK_FBO_4 = SINK_FBO_3 + 1;
  static constexpr int SINK_FBO_END = SINK_FBO_4;

protected:
  std::string name;
  ModConfig config;
  ofParameterGroup parameters;
  virtual void initParameters() = 0;
  Connections connections;
  template<typename T> void emit(int sourceId, const T& value);
  FboPtrs fboPtrs;

};


} // ofxMarkSynth
