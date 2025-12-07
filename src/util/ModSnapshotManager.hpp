//
//  ModSnapshotManager.hpp
//  ofxMarkSynth
//
//  Snapshot system for saving/recalling Mod parameter states during performance
//

#pragma once

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "nlohmann/json.hpp"
#include "Mod.hpp"



namespace ofxMarkSynth {



class Synth;

class ModSnapshotManager {
public:
    static constexpr int NUM_SLOTS = 8;
    
    // Parameter values stored as strings (using ofParameter's toString/fromString)
    using ParamMap = std::unordered_map<std::string, std::string>;
    
    struct Snapshot {
        std::string name;
        std::string timestamp;
        // modParams[modName][paramName] = paramValueString
        std::unordered_map<std::string, ParamMap> modParams;
    };
    
    ModSnapshotManager() = default;
    
    Snapshot capture(const std::string& name, const std::vector<ModPtr>& selectedMods);
    
    // Apply snapshot to Synth (only affects named Mods)
    // Returns set of affected Mod names for visual feedback
    std::unordered_set<std::string> apply(std::shared_ptr<Synth> synth, const Snapshot& snapshot);
    
    // Undo last apply (restores previous state)
    // Returns set of affected Mod names for visual feedback
    std::unordered_set<std::string> undo(std::shared_ptr<Synth> synth);
    bool canUndo() const { return undoSnapshot.has_value(); }
    
    void saveToSlot(int slot, const Snapshot& snapshot);
    std::optional<Snapshot> getSlot(int slot) const;
    bool isSlotOccupied(int slot) const;
    void clearSlot(int slot);
    
    bool saveToFile(const std::string& synthName);
    bool loadFromFile(const std::string& synthName);
    static std::string getSnapshotFilePath(const std::string& synthName);
    
private:
    std::array<std::optional<Snapshot>, NUM_SLOTS> slots;
    std::optional<Snapshot> undoSnapshot;  // Single level of undo
    
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);
    static nlohmann::json snapshotToJson(const Snapshot& snapshot);
    static Snapshot snapshotFromJson(const nlohmann::json& j);
};



} // namespace ofxMarkSynth
