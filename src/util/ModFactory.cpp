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
    auto sourceAudioPathPtr = r.get<std::filesystem::path>("sourceAudioPath");
    auto micDeviceNamePtr = r.get<std::string>("micDeviceName");
    auto recordAudioPtr = r.get<bool>("recordAudio");
    auto recordingPathPtr = r.get<std::filesystem::path>("recordingPath");
    if (sourceAudioPathPtr && !sourceAudioPathPtr->empty()) {
      return std::make_shared<AudioDataSourceMod>(s.get(), n, std::move(c), *sourceAudioPathPtr);
    } else if (micDeviceNamePtr && recordAudioPtr && recordingPathPtr) {
      return std::make_shared<AudioDataSourceMod>(s.get(), n, std::move(c), *micDeviceNamePtr, *recordAudioPtr, *recordingPathPtr);
    }
    ofLogError("ModFactory") << "AudioDataSourceMod requires either 'sourceVideoPath' or 'micDeviceName', 'recordAudio', 'recordingPath' resources";
    return nullptr;
  });
  
  registerType("StaticTextSource", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<StaticTextSourceMod>(s.get(), n, std::move(c));
  });
  
  registerType("TextSource", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    auto textSourcesPathPtr = r.get<std::string>("textSourcesPath");
    if (!textSourcesPathPtr) {
      ofLogError("ModFactory") << "TextSource requires 'textSourcesPath' resource (base directory)";
      return nullptr;
    }
    return std::make_shared<TextSourceMod>(s.get(), n, std::move(c), *textSourcesPathPtr);
  });
  
  registerType("TimerSource", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<TimerSourceMod>(s.get(), n, std::move(c));
  });
  
  registerType("RandomFloatSource", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<RandomFloatSourceMod>(s.get(), n, std::move(c),
                                                  std::pair<float, float>{0.0f, 1.0f}, // min range
                                                  std::pair<float, float>{0.0f, 1.0f}, // max range
                                                  0); // seed
  });
  
  registerType("RandomHslColor", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<RandomHslColorMod>(s.get(), n, std::move(c));
  });

  // TODO: make configurable (see Mod)
  registerType("RandomVecSource", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<RandomVecSourceMod>(s.get(), n, std::move(c));
  });
  
  registerType("VideoFlowSource", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    auto sourceVideoPathPtr = r.get<std::filesystem::path>("sourceVideoPath");
    auto sourceVideoMutePtr = r.get<bool>("sourceVideoMute");
    return std::make_shared<VideoFlowSourceMod>(s.get(), n, std::move(c), *sourceVideoPathPtr, *sourceVideoMutePtr);
  });
  
  // Register all process mods
  registerType("Cluster", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<ClusterMod>(s.get(), n, std::move(c));
  });
  
  registerType("Fluid", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<FluidMod>(s.get(), n, std::move(c));
  });
  
  registerType("FluidRadialImpulse", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<FluidRadialImpulseMod>(s.get(), n, std::move(c));
  });
  
  registerType("MultiplyAdd", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<MultiplyAddMod>(s.get(), n, std::move(c));
  });
  
  registerType("ParticleField", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<ParticleFieldMod>(s.get(), n, std::move(c));
  });
  
  registerType("ParticleSet", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<ParticleSetMod>(s.get(), n, std::move(c));
  });
  
  registerType("Path", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<PathMod>(s.get(), n, std::move(c));
  });
  
  registerType("PixelSnapshot", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<PixelSnapshotMod>(s.get(), n, std::move(c));
  });
  
  registerType("Smear", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<SmearMod>(s.get(), n, std::move(c));
  });
  
  registerType("SoftCircle", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<SoftCircleMod>(s.get(), n, std::move(c));
  });
  
  // Register all sink mods
  registerType("Collage", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<CollageMod>(s.get(), n, std::move(c));
  });
  
  registerType("DividedArea", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<DividedAreaMod>(s.get(), n, std::move(c));
  });
  
  registerType("Introspector", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<IntrospectorMod>(s.get(), n, std::move(c));
  });
  
  registerType("SandLine", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<SandLineMod>(s.get(), n, std::move(c));
  });
  
  registerType("Text", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    auto fontPathPtr = r.get<std::filesystem::path>("fontPath");
    if (!fontPathPtr) {
      ofLogError("ModFactory") << "TextMod requires 'fontPath' resource";
      return nullptr;
    }
    return std::make_shared<TextMod>(s.get(), n, std::move(c), *fontPathPtr);
  });
  
  registerType("SomPalette", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<SomPaletteMod>(s.get(), n, std::move(c));
  });
  
  // Register all layer mods
  registerType("Fade", [](std::shared_ptr<Synth> s, const std::string& n, ModConfig c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<FadeMod>(s.get(), n, std::move(c));
  });
  
  ofLogNotice("ModFactory") << "Initialized " << getRegistry().size() << " built-in Mod types";
}



} // namespace ofxMarkSynth
