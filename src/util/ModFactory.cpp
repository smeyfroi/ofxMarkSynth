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
                         Synth* synth,
                         const std::string& name,
                         ModConfig&& config,
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

  registerType("AudioDataSource", [](Synth* s, const std::string& n, ModConfig&& c, const ResourceManager& r) -> ModPtr {
    auto rootSourceMaterialPathPtr = r.get<std::filesystem::path>("rootSourceMaterialPath");
    auto sourceMaterialPathPtr = r.get<std::filesystem::path>("sourceMaterialPath");
    auto micDeviceNamePtr = r.get<std::string>("micDeviceName");
    auto recordAudioPtr = r.get<bool>("recordAudio");
    auto recordingPathPtr = r.get<std::filesystem::path>("recordingPath");
    if (rootSourceMaterialPathPtr && sourceMaterialPathPtr) {
      return std::make_shared<AudioDataSourceMod>(s, n, std::move(c), *rootSourceMaterialPathPtr, *sourceMaterialPathPtr);
    } else if (micDeviceNamePtr && recordAudioPtr && recordingPathPtr) {
      return std::make_shared<AudioDataSourceMod>(s, n, std::move(c), *micDeviceNamePtr, *recordAudioPtr, *recordingPathPtr);
    }
    ofLogError("ModFactory") << "AudioDataSourceMod requires either 'rootSourceMaterialPath' and 'sourceMaterialPath', or 'micDeviceName', 'recordAudio', and 'recordingPath' resources";
    return nullptr;
  });
  
//  registerType("RandomFloatSource", [](Synth* s, const std::string& n, ModConfig&& c, const ResourceManager& r) -> ModPtr {
//    // TODO: RandomFloatSourceMod has complex constructor with ranges - need to extract from config or use defaults
//    return std::make_shared<RandomFloatSourceMod>(s, n, std::move(c), 
//                                                   std::pair<float, float>{0.0f, 1.0f},
//                                                   std::pair<float, float>{0.0f, 1.0f});
//  });
//  
//  registerType("RandomHslColor", [](Synth* s, const std::string& n, ModConfig&& c, const ResourceManager& r) -> ModPtr {
//    return std::make_shared<RandomHslColorMod>(s, n, std::move(c));
//  });
//  
//  registerType("RandomVecSource", [](Synth* s, const std::string& n, ModConfig&& c, const ResourceManager& r) -> ModPtr {
//    // Default to vec2 (dimension=2)
//    int dimension = 2;
//    auto it = c.find("Dimension");
//    if (it != c.end()) {
//      dimension = std::stoi(it->second);
//    }
//    return std::make_shared<RandomVecSourceMod>(s, n, std::move(c), dimension);
//  });
//  
//  registerType("VideoFlowSource", [](Synth* s, const std::string& n, ModConfig&& c, const ResourceMap& r) -> ModPtr {
//    return std::make_shared<VideoFlowSourceMod>(s, n, std::move(c));
//  });
  
  // Register all process mods
  registerType("Cluster", [](Synth* s, const std::string& n, ModConfig&& c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<ClusterMod>(s, n, std::move(c));
  });
  
  registerType("Fluid", [](Synth* s, const std::string& n, ModConfig&& c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<FluidMod>(s, n, std::move(c));
  });
  
  registerType("FluidRadialImpulse", [](Synth* s, const std::string& n, ModConfig&& c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<FluidRadialImpulseMod>(s, n, std::move(c));
  });
  
  registerType("MultiplyAdd", [](Synth* s, const std::string& n, ModConfig&& c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<MultiplyAddMod>(s, n, std::move(c));
  });
  
  registerType("ParticleField", [](Synth* s, const std::string& n, ModConfig&& c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<ParticleFieldMod>(s, n, std::move(c));
  });
  
  registerType("ParticleSet", [](Synth* s, const std::string& n, ModConfig&& c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<ParticleSetMod>(s, n, std::move(c));
  });
  
  registerType("Path", [](Synth* s, const std::string& n, ModConfig&& c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<PathMod>(s, n, std::move(c));
  });
  
  registerType("PixelSnapshot", [](Synth* s, const std::string& n, ModConfig&& c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<PixelSnapshotMod>(s, n, std::move(c));
  });
  
  registerType("Smear", [](Synth* s, const std::string& n, ModConfig&& c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<SmearMod>(s, n, std::move(c));
  });
  
  registerType("SoftCircle", [](Synth* s, const std::string& n, ModConfig&& c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<SoftCircleMod>(s, n, std::move(c));
  });
  
  // Register all sink mods
  registerType("Collage", [](Synth* s, const std::string& n, ModConfig&& c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<CollageMod>(s, n, std::move(c));
  });
  
  registerType("DividedArea", [](Synth* s, const std::string& n, ModConfig&& c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<DividedAreaMod>(s, n, std::move(c));
  });
  
  registerType("Introspector", [](Synth* s, const std::string& n, ModConfig&& c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<IntrospectorMod>(s, n, std::move(c));
  });
  
  registerType("SandLine", [](Synth* s, const std::string& n, ModConfig&& c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<SandLineMod>(s, n, std::move(c));
  });
  
  registerType("SomPalette", [](Synth* s, const std::string& n, ModConfig&& c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<SomPaletteMod>(s, n, std::move(c));
  });
  
  // Register all layer mods
  registerType("Fade", [](Synth* s, const std::string& n, ModConfig&& c, const ResourceManager& r) -> ModPtr {
    return std::make_shared<FadeMod>(s, n, std::move(c));
  });
  
  ofLogNotice("ModFactory") << "Initialized " << getRegistry().size() << " built-in Mod types";
}



} // namespace ofxMarkSynth
