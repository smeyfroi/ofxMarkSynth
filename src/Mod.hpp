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


// See ofGLUtils ofGetGLInternalFormat
#ifdef TARGET_MAC
constexpr GLint FLOAT_A_MODE = GL_RGBA32F;
constexpr GLint FLOAT_MODE = GL_RGB32F;
constexpr GLint INT_A_MODE = GL_RGBA8;
constexpr GLint INT_MODE = GL_RGB8;
#else
constexpr GLint FLOAT_A_MODE = GL_RGBA;
constexpr GLint FLOAT_MODE = GL_RGB;
constexpr GLint INT_A_MODE = GL_RGBA;
constexpr GLint INT_MODE = GL_RGB;
#endif


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


// NOTE: A Mod will emit its FboPtrs when they are received, which means
// that dependents need to be hooked up BEFORE the FboPtrs are sent to Mods.
class Mod {
  
public:
  Mod(const std::string& name, const ModConfig&& config);
  virtual ~Mod() = default;
  virtual void update() {};
  virtual void draw() {};
  virtual bool keyPressed(int key) { return false; };
  ofParameterGroup& getParameterGroup();
  void addSink(int sourceId, ModPtr sinkModPtr, int sinkId);
  bool hasSinkFor(int sourceId); // can't be const because connections is not mutable
  virtual float bidToReceive(int sinkId) { return 0.0; };
  virtual void receive(int sinkId, const glm::vec1& point);
  virtual void receive(int sinkId, const glm::vec2& point);
  virtual void receive(int sinkId, const glm::vec3& point);
  virtual void receive(int sinkId, const glm::vec4& point);
  virtual void receive(int sinkId, const float& value);
  virtual void receive(int sinkId, const FboPtr& fboPtr);
  virtual void receive(int sinkId, const ofFloatPixels& pixels);
  virtual void receive(int sinkId, const ofPath& path);
  virtual void receive(int sinkId, const ofFbo& fbo);

  static constexpr int SOURCE_FBO_BEGIN = -100;
  static constexpr int SOURCE_FBO = SOURCE_FBO_BEGIN;
  static constexpr int SOURCE_FBO_2 = SOURCE_FBO + 1;
  static constexpr int SOURCE_FBO_3 = SOURCE_FBO_2 + 1;
  static constexpr int SOURCE_FBO_4 = SOURCE_FBO_3 + 1;
  static constexpr int SOURCE_FBO_5 = SOURCE_FBO_4 + 1;
  static constexpr int SOURCE_FBO_END = SOURCE_FBO_5;
  
  static constexpr int SINK_FBO_BEGIN = -200;
  static constexpr int SINK_FBO = SINK_FBO_BEGIN;
  static constexpr int SINK_FBO_2 = SINK_FBO + 1;
  static constexpr int SINK_FBO_3 = SINK_FBO_2 + 1;
  static constexpr int SINK_FBO_4 = SINK_FBO_3 + 1;
  static constexpr int SINK_FBO_5 = SINK_FBO_4 + 1;
  static constexpr int SINK_FBO_END = SINK_FBO_5;

  static constexpr int SINK_AUDIO_ONSET = -300;
  static constexpr int SINK_AUDIO_TIMBRE_CHANGE = -301;

  std::string name;
  
protected:
  ModConfig config;
  ofParameterGroup parameters;
  virtual void initParameters() = 0;
  Connections connections;
  template<typename T> void emit(int sourceId, const T& value);
  FboPtrs fboPtrs;
};


} // ofxMarkSynth
