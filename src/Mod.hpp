//
//  Mod.hpp
//  example_simple
//
//  Created by Steve Meyfroidt on 02/05/2025.
//

#pragma once

#include "PingPongFbo.h"
#include "Gui.h"
#include "NodeEditorLayout.hpp"
#include "NodeEditorLayoutSerializer.hpp"
#include "ofParameter.h"
#include "ParamController.h"
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>



// NOTE: This feels like it should be built on `ofEvent` not the custom emit/receive impl here



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



using FboPtr = std::shared_ptr<PingPongFbo>;

struct DrawingLayer {
  int id;
  std::string name;
  FboPtr fboPtr;
  bool clearOnUpdate;
  ofBlendMode blendMode;
  bool isDrawn;
  bool isOverlay;
  DrawingLayer() : id(nextId++) {}
  DrawingLayer(const std::string& name_, FboPtr fboPtr_, bool clearOnUpdate_,
               ofBlendMode blendMode_, bool isDrawn_, bool isOverlay_ = false)
  : id(nextId++), name(name_), fboPtr(fboPtr_), clearOnUpdate(clearOnUpdate_),
  blendMode(blendMode_), isDrawn(isDrawn_), isOverlay(isOverlay_) {}
private:
  inline static int nextId = -1000; // negative to avoid clashing with Mod ids
};

using DrawingLayerPtr = std::shared_ptr<DrawingLayer>;
using DrawingLayerPtrs = std::vector<DrawingLayerPtr>;
using NamedDrawingLayerPtrs = std::unordered_map<std::string, DrawingLayerPtrs>;



class Mod;
using ModConfig = std::unordered_map<std::string, std::string>;
using ModPtr = std::shared_ptr<Mod>;
using ModPtrs = std::vector<ModPtr>;

using SinkId = int;
using Sinks = std::vector<std::pair<ModPtr, SinkId>>;
using SourceId = int;
using Connections = std::unordered_map<SourceId, std::unique_ptr<Sinks>>;



static constexpr std::string DEFAULT_DRAWING_LAYER_PTR_NAME { "default" };
struct ModDrawingLayerNamePair {
  ModPtr modPtr;
  std::string name { DEFAULT_DRAWING_LAYER_PTR_NAME };
};
void assignDrawingLayerPtrToMods(DrawingLayerPtr drawingLayerPtr, std::initializer_list<ModDrawingLayerNamePair> modFboNamePairs);

struct SinkSpec {
  ModPtr sinkModPtr;
  int sinkId;
};
struct ConnectionsSpec {
  int sourceId;
  std::initializer_list<SinkSpec> sinkModFboNamePairs;
};
void connectSourceToSinks(ModPtr sourceModPtr,
                          std::initializer_list<ConnectionsSpec> connectionsSpec);



class Synth;
class Intent;



class Mod : public std::enable_shared_from_this<Mod> {
  
public:
  Mod(std::shared_ptr<Synth> synth, const std::string& name, ModConfig config);
  virtual ~Mod() = default;
  virtual void shutdown() {};
  virtual void doneModLoad() {}
  virtual void update() {};
  virtual void draw() {};
  virtual void drawOverlay() {};
  virtual bool keyPressed(int key) { return false; };
  ofParameterGroup& getParameterGroup();
  
  virtual std::optional<std::reference_wrapper<ofAbstractParameter>> findParameterByNamePrefix(const std::string& name);
  int getId() const;
  void setName(const std::string& name_) { name = name_; }
  const std::string& getName() const;
  
  virtual float getAgency() const;
  virtual void applyIntent(const Intent& intent, float intentStrength) {};
  
  int getSourceId(const std::string& sourceName);
  
  int getSinkId(const std::string& sinkName);
  void connect(int sourceId, ModPtr sinkModPtr, int sinkId);
  virtual void receive(int sinkId, const glm::vec2& point);
  virtual void receive(int sinkId, const glm::vec3& point);
  virtual void receive(int sinkId, const glm::vec4& point);
  virtual void receive(int sinkId, const float& value);
  virtual void receive(int sinkId, const ofFloatPixels& pixels);
  virtual void receive(int sinkId, const ofPath& path);
  virtual void receive(int sinkId, const ofFbo& fbo);
  virtual void receive(int sinkId, const ofTexture& texture);
  virtual void receive(int sinkId, const std::string& text);
  
  void receiveDrawingLayerPtr(const std::string& name, const DrawingLayerPtr drawingLayerPtr);
  std::optional<DrawingLayerPtr> getCurrentNamedDrawingLayerPtr(const std::string& name);
  
  static constexpr int SINK_CHANGE_LAYER = -300;
  
  // TODO: clean all this up
  friend class Gui;
  friend class NodeEditorLayout;
  friend class NodeEditorLayoutSerializer;
  std::map<std::string, BaseParamController*> sourceNameControllerPtrMap; // for the GUI to access
  
protected:
  std::string name;
  
  std::weak_ptr<Synth> synthPtr; // parent Synth (may be expired)
  
  ModConfig config;
  ofParameterGroup parameters;
  virtual void initParameters() = 0;
  
  std::map<std::string, int> sourceNameIdMap;
  std::map<std::string, int> sinkNameIdMap;
  Connections connections;
  template<typename T> void emit(int sourceId, const T& value);
  
  std::optional<DrawingLayerPtr> getNamedDrawingLayerPtr(const std::string& name, int index);
  std::optional<std::string> getRandomLayerName();
  void changeDrawingLayer();
  void changeDrawingLayer(const std::string& layerName);
  void resetDrawingLayer();
  void resetDrawingLayer(const std::string& layerName);
  void disableDrawingLayer();
  void disableDrawingLayer(const std::string& layerName);
  
  void registerControllerForSource(const std::string& sourceName, BaseParamController& controller);
  template <typename T>
  void registerControllerForSource(ofParameter<T>& param, ParamController<T>& controller) {
    registerControllerForSource(param.getName(), controller);
  }
  
  std::shared_ptr<Synth> getSynth() const;
  void syncControllerAgencies();
  
private:
  NamedDrawingLayerPtrs namedDrawingLayerPtrs; // named FBOs provided by the Synth that can be drawn on
  std::unordered_map<std::string, int> currentDrawingLayerIndices; // index < 0 means don't draw
  int id;
  static int nextId;
};



} // ofxMarkSynth
