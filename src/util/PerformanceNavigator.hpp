//
//  PerformanceNavigator.hpp
//  ofxMarkSynth
//
//  Performance config navigation with press-and-hold safety
//

#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <memory>
#include <cstdint>



namespace ofxMarkSynth {



class Synth;



class PerformanceNavigator {
  
public:
  enum class HoldAction { NONE, NEXT, PREV, JUMP };
  enum class HoldSource { NONE, KEYBOARD, MOUSE };
  
  explicit PerformanceNavigator(Synth* synth);
  bool keyPressed(int key);
  bool keyReleased(int key);
  
  // Load configs from folder
  void loadFromFolder(const std::filesystem::path& folderPath);
  
  // State accessors
  const std::vector<std::string>& getConfigs() const { return configs; }
  int getCurrentIndex() const { return currentIndex; }
  int getConfigCount() const { return static_cast<int>(configs.size()); }
  bool hasConfigs() const { return !configs.empty(); }
  std::string getCurrentConfigName() const;
  std::string getConfigName(int index) const;
  const std::filesystem::path& getFolderPath() const { return folderPath; }
  
  // Actions (called when hold completes)
  void next();
  void prev();
  void jumpTo(int index);
  
  // Load first config if available (call after Synth is fully initialized)
  void loadFirstConfigIfAvailable();
  
  // Hold management (called from key/mouse events)
  void beginHold(HoldAction action, HoldSource source, int jumpIndex = -1);
  void endHold(HoldSource source);
  void update();  // Check timer, trigger if threshold met
  
  // For UI drawing
  float getHoldProgress() const;        // 0.0 to 1.0
  HoldAction getActiveHold() const { return activeHold; }
  HoldSource getActiveHoldSource() const { return holdSource; }
  int getJumpTargetIndex() const { return jumpTargetIndex; }
  bool isHolding() const { return activeHold != HoldAction::NONE; }
  
  // Key codes for navigation
  static constexpr int KEY_NEXT = 57358;  // OF_KEY_RIGHT
  static constexpr int KEY_PREV = 57356;  // OF_KEY_LEFT
  
  // Hold threshold
  static constexpr uint64_t HOLD_THRESHOLD_MS = 400;
  static constexpr uint64_t COOLDOWN_MS = 500;  // Cooldown after successful action
  
private:
  Synth* synth;
  std::vector<std::string> configs;      // Full paths
  std::filesystem::path folderPath;
  int currentIndex = -1;                 // -1 means no config loaded
  
  // Hold state
  HoldAction activeHold = HoldAction::NONE;
  HoldSource holdSource = HoldSource::NONE;
  int jumpTargetIndex = -1;
  uint64_t holdStartTime = 0;
  bool actionTriggered = false;          // Prevent re-triggering while still holding
  uint64_t lastActionTime = 0;           // For cooldown after successful action
  
  // Load first config on init
  void loadCurrentConfig();
};



} // namespace ofxMarkSynth
