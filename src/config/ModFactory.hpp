//
//  ModFactory.hpp
//  ofxMarkSynth
//
//  Created for config serialization system
//

#pragma once

#include <functional>
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <any>



namespace ofxMarkSynth {



class Synth;
class Mod;
using ModPtr = std::shared_ptr<Mod>;
using ModConfig = std::unordered_map<std::string, std::string>;

// Resource map for Mods that need external dependencies
class ResourceManager {
  std::unordered_map<std::string, std::any> resources;
  
public:
  template<typename T>
  void add(const std::string& name, T resource) {
    resources[name] = std::make_shared<T>(std::move(resource));
  }
  
  template<typename T>
  std::shared_ptr<T> get(const std::string& name) const {
    auto it = resources.find(name);
    if (it == resources.end()) {
      return nullptr;
    }
    try {
      return std::any_cast<std::shared_ptr<T>>(it->second);
    } catch (const std::bad_any_cast&) {
      return nullptr;
    }
  }
  
  bool has(const std::string& name) const {
    return resources.find(name) != resources.end();
  }
};

using ModCreatorFn = std::function<ModPtr(std::shared_ptr<Synth>, const std::string&, ModConfig, const ResourceManager&)>;



class ModFactory {
public:
  // Register a Mod type with its creator function
  static void registerType(const std::string& typeName, ModCreatorFn creator);
  
  // Create a Mod by type name
  static ModPtr create(const std::string& typeName,
                      std::shared_ptr<Synth> synth,
                      const std::string& name,
                       ModConfig config,
                       const ResourceManager& resources);
  
  static bool isRegistered(const std::string& typeName);
  static std::vector<std::string> getRegisteredTypes();
  static void initializeBuiltinTypes();
  
private:
  static std::unordered_map<std::string, ModCreatorFn>& getRegistry();
};



} // namespace ofxMarkSynth
