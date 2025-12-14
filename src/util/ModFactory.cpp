//
//  ModFactory.cpp
//  ofxMarkSynth
//
//  Created for config serialization system
//

#include "ModFactory.hpp"
#include "Synth.hpp"
#include "ofLog.h"
#include "ofxMarkSynth.h"
#include "ofxAudioData.h"



namespace ofxMarkSynth {



std::unordered_map<std::string, ModCreatorFn>& ModFactory::getRegistry() {
  static std::unordered_map<std::string, ModCreatorFn> registry;
  return registry;
}

void ModFactory::registerType(const std::string& typeName, ModCreatorFn creator) {
  getRegistry()[typeName] = creator;
  ofLogVerbose("ModFactory") << "Registered type: " << typeName;
}

ModPtr ModFactory::create(const std::string& typeName,
                         std::shared_ptr<Synth> synth,
                         const std::string& name,
                         ModConfig config,
                         const ResourceManager& resources) {
  auto& registry = getRegistry();
  auto it = registry.find(typeName);
  
  if (it == registry.end()) {
    ofLogError("ModFactory") << "Unknown Mod type: " << typeName;
    return nullptr;
  }
  
  try {
    auto modPtr = it->second(synth, name, std::move(config), resources);
    synth->addMod(modPtr);
    return modPtr;
  } catch (const std::exception& e) {
    ofLogError("ModFactory") << "Failed to create Mod '" << name << "' of type '" << typeName << "': " << e.what();
    return nullptr;
  }
}

bool ModFactory::isRegistered(const std::string& typeName) {
  return getRegistry().find(typeName) != getRegistry().end();
}

std::vector<std::string> ModFactory::getRegisteredTypes() {
  std::vector<std::string> types;
  for (const auto& pair : getRegistry()) {
    types.push_back(pair.first);
  }
  return types;
}

void ModFactory::initializeBuiltinTypes() {

  registerType("AudioDataSource", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
  (void)r;
  if (!s) {
    ofLogError("ModFactory") << "AudioDataSource requires a valid Synth";
    return nullptr;
  }

  const auto& audioClient = s->getAudioAnalysisClient();
  if (!audioClient) {
    ofLogError("ModFactory") << "AudioDataSource requires Synth-owned audio client";
    return nullptr;
  }

  return std::make_shared<AudioDataSourceMod>(s, n, std::move(c), audioClient);
});
  
  registerType("StaticTextSource", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<StaticTextSourceMod>(s, n, std::move(c));
  });
  
  registerType("TextSource", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    auto textSourcesPathPtr = r.get<std::string>("textSourcesPath");
    if (!textSourcesPathPtr) {
      ofLogError("ModFactory") << "TextSource requires 'textSourcesPath' resource (base directory)";
      return nullptr;
    }
    return std::make_shared<TextSourceMod>(s, n, std::move(c), *textSourcesPathPtr);
  });
  
  registerType("TimerSource", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<TimerSourceMod>(s, n, std::move(c));
  });
  
  registerType("RandomFloatSource", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<RandomFloatSourceMod>(s, n, std::move(c),
                                                  std::pair<float, float>{0.0f, 1.0f}, // min range
                                                  std::pair<float, float>{0.0f, 1.0f}, // max range
                                                  0); // seed
  });
  
  registerType("RandomHslColor", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<RandomHslColorMod>(s, n, std::move(c));
  });

  // TODO: make configurable (see Mod)
  registerType("RandomVecSource", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<RandomVecSourceMod>(s, n, std::move(c));
  });
  
  registerType("VideoFlowSource", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    auto sourceVideoPathPtr = r.get<std::filesystem::path>("sourceVideoPath");
    auto sourceVideoMutePtr = r.get<bool>("sourceVideoMute");
    auto sourceVideoStartPositionPtr = r.get<std::string>("sourceVideoStartPosition");
    auto cameraDeviceIdPtr = r.get<int>("cameraDeviceId");
    auto videoSizePtr = r.get<glm::vec2>("videoSize");
    auto saveRecordingPtr = r.get<bool>("saveRecording");
    auto recordingPathPtr = r.get<std::filesystem::path>("videoRecordingPath");
    if (sourceVideoPathPtr && !sourceVideoPathPtr->empty() && sourceVideoMutePtr) {
      std::string startPosition = sourceVideoStartPositionPtr ? *sourceVideoStartPositionPtr : "";
      return std::make_shared<VideoFlowSourceMod>(s, n, std::move(c), *sourceVideoPathPtr, *sourceVideoMutePtr, startPosition);
    } else if (cameraDeviceIdPtr && videoSizePtr && saveRecordingPtr && recordingPathPtr) {
      return std::make_shared<VideoFlowSourceMod>(s, n, std::move(c), *cameraDeviceIdPtr, *videoSizePtr, *saveRecordingPtr, *recordingPathPtr);
    }
    ofLogError("ModFactory") << "VideoFlowSource requires ('sourceVideoPath', 'sourceVideoMute') or ('cameraDeviceId', 'videoSize', 'saveRecording', 'videoRecordingPath') resources";
    return nullptr;
  });
  
  // Register all process mods
  registerType("Cluster", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<ClusterMod>(s, n, std::move(c));
  });
  
  registerType("Fluid", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<FluidMod>(s, n, std::move(c));
  });
  
  registerType("FluidRadialImpulse", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<FluidRadialImpulseMod>(s, n, std::move(c));
  });
  
  registerType("MultiplyAdd", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<MultiplyAddMod>(s, n, std::move(c));
  });
  
  registerType("ParticleField", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<ParticleFieldMod>(s, n, std::move(c));
  });
  
  registerType("ParticleSet", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<ParticleSetMod>(s, n, std::move(c));
  });
  
  registerType("Path", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<PathMod>(s, n, std::move(c));
  });
  
  registerType("PixelSnapshot", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<PixelSnapshotMod>(s, n, std::move(c));
  });
  
  registerType("Smear", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<SmearMod>(s, n, std::move(c));
  });
  
  registerType("SoftCircle", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<SoftCircleMod>(s, n, std::move(c));
  });
  
  // Register all sink mods
  registerType("Collage", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<CollageMod>(s, n, std::move(c));
  });
  
  registerType("DividedArea", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<DividedAreaMod>(s, n, std::move(c));
  });
  
  registerType("Introspector", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<IntrospectorMod>(s, n, std::move(c));
  });
  
  registerType("SandLine", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<SandLineMod>(s, n, std::move(c));
  });
  
  registerType("Text", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    auto fontPathPtr = r.get<std::filesystem::path>("fontPath");
    if (!fontPathPtr) {
      ofLogError("ModFactory") << "TextMod requires 'fontPath' resource";
      return nullptr;
    }
    return std::make_shared<TextMod>(s, n, std::move(c), *fontPathPtr);
  });
  
  registerType("SomPalette", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<SomPaletteMod>(s, n, std::move(c));
  });
  
  // Register all layer mods
  registerType("Fade", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<FadeMod>(s, n, std::move(c));
  });
  
  ofLogNotice("ModFactory") << "Initialized " << getRegistry().size() << " built-in Mod types";
}



} // namespace ofxMarkSynth
