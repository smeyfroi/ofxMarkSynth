//
//  ModSnapshotManager.cpp
//  ofxMarkSynth
//
//  Snapshot system for saving/recalling Mod parameter states during performance
//

#include "config/ModSnapshotManager.hpp"
#include "config/ParamMapUtil.hpp"
#include "core/Synth.hpp"
#include "ofLog.h"
#include "ofUtils.h"
#include <fstream>
#include <filesystem>



namespace ofxMarkSynth {




constexpr const char* SNAPSHOT_FOLDER_NAME = "mod-params/snapshots";

std::string ModSnapshotManager::getSnapshotFilePath(const std::string& configId) {
    if (configId.empty()) {
        return {};
    }
    return Synth::saveConfigFilePath(std::string(SNAPSHOT_FOLDER_NAME) + "/" + configId + ".json");
}

using ParamMap = ModSnapshotManager::ParamMap;

ModSnapshotManager::Snapshot ModSnapshotManager::capture(const std::string& name, 
                                                          const std::vector<ModPtr>& selectedMods) {
    Snapshot snapshot;
    snapshot.name = name;
    snapshot.timestamp = ofGetTimestampString();
    
    for (const auto& modPtr : selectedMods) {
        if (!modPtr) continue;
        
        ParamMap params;
        ParamMapUtil::serializeParameterGroup(modPtr->getParameterGroup(), params);
        
        if (!params.empty()) {
            snapshot.modParams[modPtr->getName()] = std::move(params);
        }
    }
    
    ofLogNotice("ModSnapshotManager") << "Captured snapshot '" << name << "' with " 
                                       << snapshot.modParams.size() << " mods";
    return snapshot;
}

std::unordered_set<std::string> ModSnapshotManager::apply(std::shared_ptr<Synth> synth, 
                                                           const Snapshot& snapshot) {
    std::unordered_set<std::string> affectedMods;
    
    if (!synth) {
        ofLogError("ModSnapshotManager") << "Cannot apply snapshot: null Synth";
        return affectedMods;
    }
    
    // Capture current state for undo (only for mods that will be affected)
    Snapshot undoState;
    undoState.name = "_undo_";
    undoState.timestamp = ofGetTimestampString();
    
    for (const auto& [modName, params] : snapshot.modParams) {
        try {
            ModPtr modPtr = synth->getMod(modName);
            if (!modPtr) {
                ofLogWarning("ModSnapshotManager") << "Mod '" << modName << "' not found in synth, skipping";
                continue;
            }
            
            // Save current state for undo
            ParamMap currentParams;
            ParamMapUtil::serializeParameterGroup(modPtr->getParameterGroup(), currentParams);
            undoState.modParams[modName] = std::move(currentParams);
            
            // Apply new values
            ParamMapUtil::deserializeParameterGroup(modPtr->getParameterGroup(), params);
            affectedMods.insert(modName);
            
            ofLogVerbose("ModSnapshotManager") << "Applied " << params.size() 
                                                << " parameters to mod '" << modName << "'";
        } catch (const std::exception& e) {
            ofLogError("ModSnapshotManager") << "Error applying to mod '" << modName << "': " << e.what();
        }
    }
    
    // Store undo state
    if (!affectedMods.empty()) {
        undoSnapshot = std::move(undoState);
    }
    
    ofLogNotice("ModSnapshotManager") << "Applied snapshot '" << snapshot.name 
                                       << "' to " << affectedMods.size() << " mods";
    return affectedMods;
}

std::unordered_set<std::string> ModSnapshotManager::undo(std::shared_ptr<Synth> synth) {
    std::unordered_set<std::string> affectedMods;
    
    if (!undoSnapshot.has_value()) {
        ofLogWarning("ModSnapshotManager") << "Nothing to undo";
        return affectedMods;
    }
    
    if (!synth) {
        ofLogError("ModSnapshotManager") << "Cannot undo: null Synth";
        return affectedMods;
    }
    
    // Apply the undo snapshot (don't create new undo state)
    for (const auto& [modName, params] : undoSnapshot->modParams) {
        try {
            ModPtr modPtr = synth->getMod(modName);
            if (!modPtr) {
                ofLogWarning("ModSnapshotManager") << "Mod '" << modName << "' not found for undo";
                continue;
            }
            
            ParamMapUtil::deserializeParameterGroup(modPtr->getParameterGroup(), params);
            affectedMods.insert(modName);
        } catch (const std::exception& e) {
            ofLogError("ModSnapshotManager") << "Error during undo for mod '" << modName << "': " << e.what();
        }
    }
    
    ofLogNotice("ModSnapshotManager") << "Undid changes to " << affectedMods.size() << " mods";
    
    undoSnapshot.reset();
    
    return affectedMods;
}

void ModSnapshotManager::saveToSlot(int slot, const Snapshot& snapshot) {
    if (slot < 0 || slot >= NUM_SLOTS) {
        ofLogError("ModSnapshotManager") << "Invalid slot index: " << slot;
        return;
    }
    slots[slot] = snapshot;
    ofLogNotice("ModSnapshotManager") << "Saved snapshot '" << snapshot.name << "' to slot " << slot;
}

std::optional<ModSnapshotManager::Snapshot> ModSnapshotManager::getSlot(int slot) const {
    if (slot < 0 || slot >= NUM_SLOTS) {
        return std::nullopt;
    }
    return slots[slot];
}

bool ModSnapshotManager::isSlotOccupied(int slot) const {
    if (slot < 0 || slot >= NUM_SLOTS) {
        return false;
    }
    return slots[slot].has_value();
}

void ModSnapshotManager::clearSlot(int slot) {
    if (slot < 0 || slot >= NUM_SLOTS) {
        ofLogError("ModSnapshotManager") << "Invalid slot index: " << slot;
        return;
    }
    slots[slot].reset();
    ofLogNotice("ModSnapshotManager") << "Cleared slot " << slot;
}

int ModSnapshotManager::findNameInOtherSlot(const std::string& name, int excludeSlot) const {
    for (int i = 0; i < NUM_SLOTS; ++i) {
        if (i == excludeSlot) continue;
        if (slots[i].has_value() && slots[i]->name == name) {
            return i;
        }
    }
    return -1;
}

nlohmann::json ModSnapshotManager::snapshotToJson(const Snapshot& snapshot) {
    nlohmann::json j;
    j["name"] = snapshot.name;
    j["timestamp"] = snapshot.timestamp;
    j["mods"] = nlohmann::json::object();
    
    for (const auto& [modName, params] : snapshot.modParams) {
        j["mods"][modName] = nlohmann::json::object();
        for (const auto& [paramName, paramValue] : params) {
            j["mods"][modName][paramName] = paramValue;
        }
    }
    
    return j;
}

ModSnapshotManager::Snapshot ModSnapshotManager::snapshotFromJson(const nlohmann::json& j) {
    Snapshot snapshot;
    snapshot.name = j.value("name", "");
    snapshot.timestamp = j.value("timestamp", "");
    
    if (j.contains("mods") && j["mods"].is_object()) {
        for (auto& [modName, modParams] : j["mods"].items()) {
            ParamMap params;
            for (auto& [paramName, paramValue] : modParams.items()) {
                params[paramName] = paramValue.get<std::string>();
            }
            snapshot.modParams[modName] = std::move(params);
        }
    }
    
    return snapshot;
}

nlohmann::json ModSnapshotManager::toJson() const {
    nlohmann::json j;
    j["version"] = "1.0";
    j["snapshots"] = nlohmann::json::object();
    
    for (int i = 0; i < NUM_SLOTS; ++i) {
        if (slots[i].has_value()) {
            j["snapshots"][std::to_string(i)] = snapshotToJson(*slots[i]);
        }
    }
    
    return j;
}

void ModSnapshotManager::fromJson(const nlohmann::json& j) {
    for (auto& slot : slots) {
        slot.reset();
    }

    if (j.contains("snapshots") && j["snapshots"].is_object()) {
        for (auto& [slotStr, snapshotJson] : j["snapshots"].items()) {
            try {
                int slot = std::stoi(slotStr);
                if (slot >= 0 && slot < NUM_SLOTS) {
                    slots[slot] = snapshotFromJson(snapshotJson);
                }
            } catch (const std::exception& e) {
                ofLogWarning("ModSnapshotManager") << "Invalid slot key: " << slotStr;
            }
        }
    }
}

bool ModSnapshotManager::saveToFile(const std::string& configId) {
    try {
        std::string filepath = getSnapshotFilePath(configId);
        std::filesystem::path dir = std::filesystem::path(filepath).parent_path();
        std::filesystem::create_directories(dir);
        

        nlohmann::json j = toJson();
        
        std::ofstream file(filepath);
        if (!file.is_open()) {
            ofLogError("ModSnapshotManager") << "Failed to open file for writing: " << filepath;
            return false;
        }
        
        file << j.dump(2);  // Pretty print with 2-space indent
        file.close();
        
        ofLogNotice("ModSnapshotManager") << "Saved snapshots to: " << filepath;
        return true;
    } catch (const std::exception& e) {
        ofLogError("ModSnapshotManager") << "Exception saving snapshots: " << e.what();
        return false;
    }
}

bool ModSnapshotManager::loadFromFile(const std::string& configId) {
    std::string filepath = getSnapshotFilePath(configId);

    for (auto& slot : slots) {
        slot.reset();
    }

    undoSnapshot.reset();

    if (!std::filesystem::exists(filepath)) {
        ofLogNotice("ModSnapshotManager") << "No snapshot file found: " << filepath;
        return false;
    }
    
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            ofLogError("ModSnapshotManager") << "Failed to open file for reading: " << filepath;
            return false;
        }
        
        nlohmann::json j;
        file >> j;
        file.close();
        
        fromJson(j);
        
        // Count loaded snapshots
        int count = 0;
        for (const auto& slot : slots) {
            if (slot.has_value()) count++;
        }
        
        ofLogNotice("ModSnapshotManager") << "Loaded " << count << " snapshots from: " << filepath;
        return true;
    } catch (const std::exception& e) {
        ofLogError("ModSnapshotManager") << "Exception loading snapshots: " << e.what();
        return false;
    }
}



} // namespace ofxMarkSynth
