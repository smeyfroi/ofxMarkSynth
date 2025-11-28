//
//  PerformanceNavigator.cpp
//  ofxMarkSynth
//
//  Performance config navigation with press-and-hold safety
//

#include "PerformanceNavigator.hpp"
#include "Synth.hpp"
#include "ofLog.h"
#include "ofUtils.h"
#include <algorithm>



namespace ofxMarkSynth {



PerformanceNavigator::PerformanceNavigator(Synth* synth_)
: synth(synth_)
{}

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

void PerformanceNavigator::loadFromFolder(const std::filesystem::path& folder) {
  configs.clear();
  folderPath = folder;
  currentIndex = -1;
  
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
  
  for (const auto& path : jsonFiles) {
    configs.push_back(path.string());
  }
  
  ofLogNotice("PerformanceNavigator") << "Loaded " << configs.size() << " configs from " << folder;
  
  if (configs.empty()) return;

  currentIndex = 0;
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
    synth->switchToConfig(configPath, false);  // No hibernation for first load
  }
}

void PerformanceNavigator::loadCurrentConfig() {
  if (currentIndex < 0 || currentIndex >= static_cast<int>(configs.size())) return;
  
  const std::string& configPath = configs[currentIndex];
  ofLogNotice("PerformanceNavigator") << "Loading config: " << getConfigName(currentIndex);
  
  if (synth) {
    synth->switchToConfig(configPath, true);  // Use hibernation fade
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
  
  // Block holds while hibernating
  if (synth && synth->isHibernating()) {
    ofLogVerbose("PerformanceNavigator") << "beginHold: blocked during hibernation";
    return;
  }
  
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
  
  ofLogVerbose("PerformanceNavigator") << "update: elapsed=" << elapsed << " threshold=" << HOLD_THRESHOLD_MS;
  
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



} // namespace ofxMarkSynth
