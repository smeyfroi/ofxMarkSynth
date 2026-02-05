//
//  PerformanceNavigator.cpp
//  ofxMarkSynth
//
//  Performance config navigation with press-and-hold safety
//

#include "config/PerformanceNavigator.hpp"
#include "core/Synth.hpp"
#include "nlohmann/json.hpp"
#include "ofLog.h"
#include "ofUtils.h"
#include "ofImage.h"
#include <algorithm>
#include <cstdlib>
#include <fstream>



namespace ofxMarkSynth {



PerformanceNavigator::PerformanceNavigator(Synth* synth_)
: synth(synth_) {
    gridConfigIndices.fill(-1);
}

bool PerformanceNavigator::keyPressed(int key) {
  if (key == OF_KEY_RIGHT) {
    beginHold(HoldAction::NEXT, HoldSource::KEYBOARD);
    return true;
  }
  if (key == OF_KEY_LEFT) {
    beginHold(HoldAction::PREV, HoldSource::KEYBOARD);
    return true;
  }
  return false;
}

bool PerformanceNavigator::keyReleased(int key) {
  if (key == OF_KEY_RIGHT || key == OF_KEY_LEFT) {
    endHold(HoldSource::KEYBOARD);
    return true;
  }
  return false;
}

namespace {

struct ParsedConfigMetadata {
    std::string description;
    std::optional<PerformanceNavigator::GridCoord> explicitGrid;
    PerformanceNavigator::RgbColor color { 128, 128, 128 };
    bool hasExplicitColor { false };
    std::shared_ptr<ofTexture> thumbnail;
};

PerformanceNavigator::RgbColor parseHexColor(const std::string& hex) {
    std::string s = ofTrim(hex);
    if (!s.empty() && s[0] == '#') {
        s = s.substr(1);
    }

    if (s.size() != 6) {
        return { 128, 128, 128 };
    }

    auto hexByte = [](const std::string& str, size_t offset) -> std::optional<uint8_t> {
        try {
            unsigned int v = std::stoul(str.substr(offset, 2), nullptr, 16);
            return static_cast<uint8_t>(v & 0xFF);
        } catch (...) {
            return std::nullopt;
        }
    };

    auto r = hexByte(s, 0);
    auto g = hexByte(s, 2);
    auto b = hexByte(s, 4);
    if (!r || !g || !b) {
        return { 128, 128, 128 };
    }

    return { *r, *g, *b };
}

constexpr int MAX_THUMBNAIL_DIM_PX = 256;

std::optional<std::filesystem::path> findThumbnailPath(const std::filesystem::path& configJsonPath) {
    const std::filesystem::path dir = configJsonPath.parent_path();
    const std::string stem = configJsonPath.stem().string();

    const std::filesystem::path jpg = dir / (stem + ".jpg");
    if (std::filesystem::exists(jpg) && std::filesystem::is_regular_file(jpg)) {
        return jpg;
    }

    const std::filesystem::path jpeg = dir / (stem + ".jpeg");
    if (std::filesystem::exists(jpeg) && std::filesystem::is_regular_file(jpeg)) {
        return jpeg;
    }

    return std::nullopt;
}

ParsedConfigMetadata parseConfigMetadata(const std::filesystem::path& filepath) {
    ParsedConfigMetadata meta;

    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return meta;
        }

        nlohmann::json j;
        file >> j;

        if (j.contains("description") && j["description"].is_string()) {
            meta.description = j["description"].get<std::string>();
        }

        if (j.contains("buttonGrid") && j["buttonGrid"].is_object()) {
            const auto& bg = j["buttonGrid"];

            if (bg.contains("x") && bg.contains("y") && bg["x"].is_number() && bg["y"].is_number()) {
                PerformanceNavigator::GridCoord gc;
                gc.x = bg["x"].get<int>();
                gc.y = bg["y"].get<int>();
                meta.explicitGrid = gc;
            }

            if (bg.contains("color") && bg["color"].is_string()) {
                meta.color = parseHexColor(bg["color"].get<std::string>());
                meta.hasExplicitColor = true;
            }
        }

        // Optional thumbnail next to config JSON: <stem>.jpg or <stem>.jpeg
        const auto thumbPathOpt = findThumbnailPath(filepath);
        if (thumbPathOpt) {
            ofImage img;
            if (!img.load(thumbPathOpt->string())) {
                ofLogError("PerformanceNavigator") << "Failed to load thumbnail: " << *thumbPathOpt;
            } else {
                const int w = static_cast<int>(img.getWidth());
                const int h = static_cast<int>(img.getHeight());
                if (w <= 0 || h <= 0) {
                    ofLogError("PerformanceNavigator") << "Invalid thumbnail dimensions: " << *thumbPathOpt;
                } else if (w > MAX_THUMBNAIL_DIM_PX || h > MAX_THUMBNAIL_DIM_PX) {
                    ofLogError("PerformanceNavigator") << "Thumbnail too large (max " << MAX_THUMBNAIL_DIM_PX << "px): "
                                                      << *thumbPathOpt << " (" << w << "x" << h << ")";
                } else {
                    auto tex = std::make_shared<ofTexture>();
                    tex->loadData(img.getPixels());
                    if (!tex->isAllocated()) {
                        ofLogError("PerformanceNavigator") << "Failed to upload thumbnail texture: " << *thumbPathOpt;
                    } else {
                        meta.thumbnail = tex;
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        ofLogVerbose("PerformanceNavigator") << "Failed to parse metadata from " << filepath << ": " << e.what();
    }

    return meta;
}

} // namespace

void PerformanceNavigator::loadFromFolder(const std::filesystem::path& folder) {
    configs.clear();
    configDescriptions.clear();
    configThumbnails.clear();
    folderPath = folder;
    currentIndex = -1;

    gridConfigIndices.fill(-1);
    configAssignedGridIndex.clear();
    configGridColors.clear();

    if (!std::filesystem::exists(folder)) {
        ofLogWarning("PerformanceNavigator") << "Folder does not exist: " << folder;
        return;
    }

    if (!std::filesystem::is_directory(folder)) {
        ofLogWarning("PerformanceNavigator") << "Path is not a directory: " << folder;
        return;
    }

    std::vector<std::filesystem::path> jsonFiles;
    for (const auto& entry : std::filesystem::directory_iterator(folder)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            jsonFiles.push_back(entry.path());
        }
    }

    std::sort(jsonFiles.begin(), jsonFiles.end(), [](const auto& a, const auto& b) {
        return a.filename().string() < b.filename().string();
    });

    std::vector<std::optional<GridCoord>> explicitGridCoords;
    explicitGridCoords.reserve(jsonFiles.size());

    RgbColor lastColor { 128, 128, 128 };

    for (const auto& path : jsonFiles) {
        configs.push_back(path.string());

        auto meta = parseConfigMetadata(path);
        configDescriptions.push_back(meta.description);
        configThumbnails.push_back(meta.thumbnail);
        explicitGridCoords.push_back(meta.explicitGrid);

        if (meta.hasExplicitColor) {
            lastColor = meta.color;
        }
        configGridColors.push_back(meta.hasExplicitColor ? meta.color : lastColor);
    }

    ofLogNotice("PerformanceNavigator") << "Loaded " << configs.size() << " configs from " << folder;

    if (configs.empty()) {
        return;
    }

    configAssignedGridIndex.assign(configs.size(), -1);
    buildConfigGrid(explicitGridCoords);

    currentIndex = 0;
}

void PerformanceNavigator::buildConfigGrid(const std::vector<std::optional<GridCoord>>& explicitGridCoords) {
    gridConfigIndices.fill(-1);

    if (configs.empty()) {
        return;
    }

    if (configAssignedGridIndex.size() != configs.size()) {
        configAssignedGridIndex.assign(configs.size(), -1);
    }

    if (explicitGridCoords.size() != configs.size()) {
        ofLogWarning("PerformanceNavigator") << "buildConfigGrid: metadata size mismatch";
    }

    // First pass: place configs with explicit (x,y).
    for (int configIdx = 0; configIdx < static_cast<int>(configs.size()); ++configIdx) {
        if (configIdx >= static_cast<int>(explicitGridCoords.size())) {
            continue;
        }

        const auto& maybeCoord = explicitGridCoords[configIdx];
        if (!maybeCoord) {
            continue;
        }

        const int x = maybeCoord->x;
        const int y = maybeCoord->y;

        if (x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT) {
            ofLogWarning("PerformanceNavigator") << "Invalid buttonGrid coords for " << configs[configIdx]
                                                 << " (" << x << "," << y << ")";
            continue;
        }

        const int cellIndex = gridXYToIndex(x, y);
        if (gridConfigIndices[cellIndex] != -1) {
            ofLogWarning("PerformanceNavigator") << "buttonGrid conflict at (" << x << "," << y << ") for " << configs[configIdx];
            continue;
        }

        gridConfigIndices[cellIndex] = configIdx;
        configAssignedGridIndex[configIdx] = cellIndex;
    }

    // Second pass: auto-assign unplaced configs (top-left → bottom-right).
    std::vector<int> freeCells;
    freeCells.reserve(GRID_CELL_COUNT);

    for (int y = 0; y < GRID_HEIGHT; ++y) {
        for (int x = 0; x < GRID_WIDTH; ++x) {
            const int cellIndex = gridXYToIndex(x, y);
            if (gridConfigIndices[cellIndex] == -1) {
                freeCells.push_back(cellIndex);
            }
        }
    }

    size_t nextFree = 0;
    for (int configIdx = 0; configIdx < static_cast<int>(configs.size()); ++configIdx) {
        if (configIdx < static_cast<int>(configAssignedGridIndex.size()) && configAssignedGridIndex[configIdx] != -1) {
            continue;
        }

        if (nextFree >= freeCells.size()) {
            ofLogWarning("PerformanceNavigator") << "No free grid cell for config index " << configIdx;
            break;
        }

        const int cellIndex = freeCells[nextFree++];
        gridConfigIndices[cellIndex] = configIdx;
        configAssignedGridIndex[configIdx] = cellIndex;
    }
}

std::string PerformanceNavigator::getCurrentConfigName() const {
  if (currentIndex < 0 || currentIndex >= static_cast<int>(configs.size())) {
    return "";
  }
  std::filesystem::path p(configs[currentIndex]);
  return p.stem().string();  // filename without extension
}

std::string PerformanceNavigator::getConfigName(int index) const {
  if (index < 0 || index >= static_cast<int>(configs.size())) {
    return "";
  }
  std::filesystem::path p(configs[index]);
  return p.stem().string();
}

std::string PerformanceNavigator::getConfigDescription(int index) const {
    if (index < 0 || index >= static_cast<int>(configDescriptions.size())) {
        return "";
    }
    return configDescriptions[index];
}

const ofTexture* PerformanceNavigator::getConfigThumbnail(int index) const {
    if (index < 0 || index >= static_cast<int>(configThumbnails.size())) {
        return nullptr;
    }
    const auto& tex = configThumbnails[index];
    return tex ? tex.get() : nullptr;
}

bool PerformanceNavigator::hasConfigThumbnail(int index) const {
    const ofTexture* tex = getConfigThumbnail(index);
    return tex != nullptr && tex->isAllocated();
}


int PerformanceNavigator::getGridConfigIndex(int x, int y) const {
    if (x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT) {
        return -1;
    }

    return gridConfigIndices[gridXYToIndex(x, y)];
}

PerformanceNavigator::RgbColor PerformanceNavigator::getConfigGridColor(int configIndex) const {
    if (configIndex < 0 || configIndex >= static_cast<int>(configGridColors.size())) {
        return { 0, 0, 0 };
    }

    return configGridColors[configIndex];
}

bool PerformanceNavigator::isConfigAssignedToGrid(int configIndex) const {
    if (configIndex < 0 || configIndex >= static_cast<int>(configAssignedGridIndex.size())) {
        return false;
    }

    return configAssignedGridIndex[configIndex] >= 0;
}

void PerformanceNavigator::next() {
  if (configs.empty()) return;
  if (currentIndex >= static_cast<int>(configs.size()) - 1) return;
  
  currentIndex++;
  loadCurrentConfig();
}

void PerformanceNavigator::prev() {
  if (configs.empty()) return;
  if (currentIndex <= 0) return;
  
  currentIndex--;
  loadCurrentConfig();
}

void PerformanceNavigator::jumpTo(int index) {
  if (configs.empty()) return;
  if (index < 0 || index >= static_cast<int>(configs.size())) return;
  if (index == currentIndex) return;  // Already at this config
  
  currentIndex = index;
  loadCurrentConfig();
}

void PerformanceNavigator::loadFirstConfigIfAvailable() {
  if (!hasConfigs() || currentIndex != 0) return;

  const std::string& configPath = configs[currentIndex];
  ofLogNotice("PerformanceNavigator") << "Loading first config (no hibernation): " << getConfigName(currentIndex);

  if (synth) {
    synth->switchToConfig(configPath, false);  // No crossfade for first load
  }
}

bool PerformanceNavigator::selectConfigByName(const std::string& name) {
  if (!hasConfigs()) return false;

  std::string trimmed = ofTrim(name);
  if (trimmed.empty()) return false;

  std::filesystem::path p(trimmed);
  std::string stem = p.stem().string();
  if (stem.empty()) stem = trimmed;

  for (int i = 0; i < static_cast<int>(configs.size()); ++i) {
    if (getConfigName(i) == stem) {
      currentIndex = i;
      return true;
    }
  }

  return false;
}

std::string PerformanceNavigator::getCurrentConfigPath() const {
  if (currentIndex < 0 || currentIndex >= static_cast<int>(configs.size())) {
    return "";
  }
  return configs[currentIndex];
}

void PerformanceNavigator::loadCurrentConfig() {
  if (currentIndex < 0 || currentIndex >= static_cast<int>(configs.size())) return;
  
  const std::string& configPath = configs[currentIndex];
  ofLogNotice("PerformanceNavigator") << "Loading config: " << getConfigName(currentIndex);
  
  // Config running time is reset in Synth::switchToConfig()
  
  if (synth) {
    synth->switchToConfig(configPath, true);  // Use crossfade transition
  }
}

void PerformanceNavigator::beginHold(HoldAction action, HoldSource source, int jumpIndex) {
  if (action == HoldAction::NONE) return;
  
  // Check cooldown period after last successful action
  uint64_t now = ofGetElapsedTimeMillis();
  if (lastActionTime > 0 && (now - lastActionTime) < COOLDOWN_MS) {
    ofLogVerbose("PerformanceNavigator") << "beginHold: in cooldown period";
    return;
  }
  
  // Allow config navigation while hibernated or paused - config loads but synth stays in current state
  
  // Ignore if already holding the same action from the same source (keyboard auto-repeat)
  if (activeHold == action && holdSource == source) {
    if (action != HoldAction::JUMP || jumpTargetIndex == jumpIndex) {
      ofLogVerbose("PerformanceNavigator") << "beginHold: ignoring repeat for action " << static_cast<int>(action);
      return;
    }
  }
  
  // Don't allow hold if action would be no-op
  if (action == HoldAction::NEXT && currentIndex >= static_cast<int>(configs.size()) - 1) {
    ofLogVerbose("PerformanceNavigator") << "beginHold: NEXT blocked, already at last config";
    return;
  }
  if (action == HoldAction::PREV && currentIndex <= 0) {
    ofLogVerbose("PerformanceNavigator") << "beginHold: PREV blocked, already at first config";
    return;
  }
  if (action == HoldAction::JUMP) {
    if (jumpIndex < 0 || jumpIndex >= static_cast<int>(configs.size())) return;
    if (jumpIndex == currentIndex) return;
  }
  
  ofLogNotice("PerformanceNavigator") << "beginHold: starting hold for action " << static_cast<int>(action) 
                                       << " source=" << static_cast<int>(source)
                                       << " currentIndex=" << currentIndex << " configs.size()=" << configs.size();
  
  activeHold = action;
  holdSource = source;
  jumpTargetIndex = (action == HoldAction::JUMP) ? jumpIndex : -1;
  holdStartTime = ofGetElapsedTimeMillis();
  actionTriggered = false;
}

void PerformanceNavigator::endHold(HoldSource source) {
  // Only end hold if it was started by the same source
  if (holdSource != source) {
    ofLogVerbose("PerformanceNavigator") << "endHold: ignoring, source mismatch (hold=" << static_cast<int>(holdSource) 
                                          << " end=" << static_cast<int>(source) << ")";
    return;
  }
  ofLogNotice("PerformanceNavigator") << "endHold called, was holding action " << static_cast<int>(activeHold);
  activeHold = HoldAction::NONE;
  holdSource = HoldSource::NONE;
  jumpTargetIndex = -1;
  actionTriggered = false;
}

void PerformanceNavigator::update() {
  if (activeHold == HoldAction::NONE) return;
  if (actionTriggered) return;  // Already triggered, waiting for release
  
  uint64_t elapsed = ofGetElapsedTimeMillis() - holdStartTime;
  if (elapsed >= HOLD_THRESHOLD_MS) {
    ofLogNotice("PerformanceNavigator") << "update: triggering action " << static_cast<int>(activeHold);
    HoldAction action = activeHold;
    int jumpIdx = jumpTargetIndex;
    
    // Reset hold state before triggering action
    activeHold = HoldAction::NONE;
    holdSource = HoldSource::NONE;
    jumpTargetIndex = -1;
    actionTriggered = false;
    
    // Record action time for cooldown
    lastActionTime = ofGetElapsedTimeMillis();
    
    switch (action) {
      case HoldAction::NEXT:
        next();
        break;
      case HoldAction::PREV:
        prev();
        break;
      case HoldAction::JUMP:
        jumpTo(jumpIdx);
        break;
      default:
        break;
    }
  }
}

float PerformanceNavigator::getHoldProgress() const {
  if (activeHold == HoldAction::NONE) return 0.0f;
  if (actionTriggered) return 1.0f;
  
  uint64_t elapsed = ofGetElapsedTimeMillis() - holdStartTime;
  float progress = static_cast<float>(elapsed) / static_cast<float>(HOLD_THRESHOLD_MS);
  return std::min(progress, 1.0f);
}

void PerformanceNavigator::setConfigDurationSec(int durationSec) {
  configDurationSec = durationSec;
}

std::optional<int> PerformanceNavigator::getTimeRemainingSec() const {
  if (configDurationSec <= 0 || !synth) return std::nullopt;
  return configDurationSec - static_cast<int>(synth->getConfigRunningTime());
}

int PerformanceNavigator::getCountdownSec() const {
  // Back-compat: returns 0 when no duration is configured.
  auto remaining = getTimeRemainingSec();
  return remaining ? *remaining : 0;
}

int PerformanceNavigator::getCountdownMinutes() const {
  return std::abs(getCountdownSec()) / 60;
}

int PerformanceNavigator::getCountdownSeconds() const {
  return std::abs(getCountdownSec()) % 60;
}

bool PerformanceNavigator::isCountdownNegative() const {
  return getCountdownSec() < 0;
}

bool PerformanceNavigator::isCountdownExpired() const {
  return configDurationSec > 0 && getCountdownSec() <= 0;
}

bool PerformanceNavigator::isConfigTimeExpired() const {
  auto remaining = getTimeRemainingSec();
  return remaining && *remaining <= 0;
}

bool PerformanceNavigator::isConfigTimeExpired(float nowSec) const {
  if (!isConfigTimeExpired()) return false;
  return static_cast<int>(nowSec * 2.0f) % 2 == 0;
}

bool PerformanceNavigator::isConfigChangeImminent(int withinSec) const {
  if (withinSec <= 0) return false;
  auto remaining = getTimeRemainingSec();
  return remaining && *remaining > 0 && *remaining <= withinSec;
}

float PerformanceNavigator::getImminentConfigChangeProgress(int withinSec) const {
  if (withinSec <= 0) return 0.0f;
  auto remaining = getTimeRemainingSec();
  if (!remaining) return 0.0f;

  // Progress ramps from 0->1 during the final `withinSec` seconds.
  float t = 1.0f - (static_cast<float>(*remaining) / static_cast<float>(withinSec));
  return ofClamp(t, 0.0f, 1.0f);
}



} // namespace ofxMarkSynth
