//
//  PerformanceNavigator.hpp
//  ofxMarkSynth
//
//  Performance config navigation with press-and-hold safety
//

#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>



class ofTexture;

namespace ofxMarkSynth {



class Synth;



class PerformanceNavigator {
  
public:
  enum class HoldAction { NONE, NEXT, PREV, JUMP };
  enum class HoldSource { NONE, KEYBOARD, MOUSE, APC_MINI };
  
  explicit PerformanceNavigator(Synth* synth);
  bool keyPressed(int key);
  bool keyReleased(int key);
  
  // Load configs from folder
  void loadFromFolder(const std::filesystem::path& folderPath);
  
  struct RgbColor {
    uint8_t r { 0 };
    uint8_t g { 0 };
    uint8_t b { 0 };
  };

  struct GridCoord {
    int x { 0 };
    int y { 0 };
  };

  static constexpr int GRID_WIDTH = 8;
  static constexpr int GRID_HEIGHT = 7;
  static constexpr int GRID_CELL_COUNT = GRID_WIDTH * GRID_HEIGHT;

  // State accessors
  const std::vector<std::string>& getConfigs() const { return configs; }
  int getCurrentIndex() const { return currentIndex; }
  int getConfigCount() const { return static_cast<int>(configs.size()); }
  bool hasConfigs() const { return !configs.empty(); }
  std::string getCurrentConfigName() const;
  std::string getConfigName(int index) const;
  std::string getConfigDescription(int index) const;
  const ::ofTexture* getConfigThumbnail(int index) const;
  bool hasConfigThumbnail(int index) const;
  const std::filesystem::path& getFolderPath() const { return folderPath; }

  // Config grid (used by GUI + controllers)
  int getGridConfigIndex(int x, int y) const;
  RgbColor getConfigGridColor(int configIndex) const;
  bool isConfigAssignedToGrid(int configIndex) const;
  
  // Actions (called when hold completes)
  void next();
  void prev();
  void jumpTo(int index);
  
  // Load first config if available (call after Synth is fully initialized)
  void loadFirstConfigIfAvailable();

  // Select config by filename stem (case-sensitive), e.g. "movement1-a" or "movement1-a.json".
  // Returns false if not found.
  bool selectConfigByName(const std::string& name);

  // Convenience accessor for current config full path.
  std::string getCurrentConfigPath() const;
  
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
  
  // Config duration and timing cues (optional, 0 means no duration specified)
  void setConfigDurationSec(int durationSec);
  int getConfigDurationSec() const { return configDurationSec; }
  bool hasConfigDuration() const { return configDurationSec > 0; }

  // Signed time remaining (negative when over time). Nullopt when no duration.
  std::optional<int> getTimeRemainingSec() const;

  // Back-compat countdown helpers (prefer getTimeRemainingSec())
  int getCountdownSec() const;  // Remaining seconds (negative if over time)
  int getCountdownMinutes() const;  // Absolute value of minutes remaining
  int getCountdownSeconds() const;  // Absolute value of seconds remaining (0-59)
  bool isCountdownNegative() const;  // True if over time
  bool isCountdownExpired() const;   // True if countdown <= 0

  // New cue helpers
  bool isConfigTimeExpired() const;              // Expired (countdown <= 0)
  bool isConfigTimeExpired(float nowSec) const;  // Expired and flash-on at nowSec
  bool isConfigChangeImminent(int withinSec) const;
  float getImminentConfigChangeProgress(int withinSec) const;
  
private:
  Synth* synth;
  std::vector<std::string> configs;      // Full paths
  std::vector<std::string> configDescriptions; // Parallel to configs
  std::vector<std::shared_ptr<::ofTexture>> configThumbnails; // Parallel to configs

  std::filesystem::path folderPath;
  int currentIndex = -1;                 // -1 means no config loaded

  // Performance config grid mapping.
  // - gridConfigIndices: size 56, maps (x,y) -> config index, or -1 if empty.
  // - configAssignedGridIndex: per-config assigned cell index, or -1.
  // - configGridColors: per-config base color (usually from buttonGrid.color).
  std::array<int, GRID_CELL_COUNT> gridConfigIndices;
  std::vector<int> configAssignedGridIndex;
  std::vector<RgbColor> configGridColors;
  
  // Hold state
  HoldAction activeHold = HoldAction::NONE;
  HoldSource holdSource = HoldSource::NONE;
  int jumpTargetIndex = -1;
  uint64_t holdStartTime = 0;
  bool actionTriggered = false;          // Prevent re-triggering while still holding
  uint64_t lastActionTime = 0;           // For cooldown after successful action
  
  // Load first config on init
  void loadCurrentConfig();

  static constexpr int gridXYToIndex(int x, int y) {
    return x + y * GRID_WIDTH;
  }
  void buildConfigGrid(const std::vector<std::optional<GridCoord>>& explicitGridCoords);
  
  // Config duration (0 = no duration specified)
  int configDurationSec { 0 };
};



} // namespace ofxMarkSynth
