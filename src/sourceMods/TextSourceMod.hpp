//
//  TextSourceMod.hpp
//  ofxMarkSynth
//
//  Unified text source supporting word/line parsing with probabilistic selection.
//  Integrates with Intent system: Chaos → Randomness
//

#pragma once

#include "core/Mod.hpp"
#include "core/ParamController.h"
#include <vector>
#include <unordered_set>
#include <filesystem>

namespace ofxMarkSynth {

/**
 * TextSourceMod - Unified text source with flexible selection strategies
 * 
 * Features:
 * - Parse mode: Split into words OR keep whole lines
 * - Randomness: Float parameter (0.0-1.0) controls selection probability
 *   - 0.0 = Pure sequential (structured, predictable)
 *   - 0.5 = Mixed (50% random, 50% sequential picks from remaining items)
 *   - 1.0 = Pure random (chaotic, unpredictable)
 * - Intent integration: Chaos dimension maps to randomness
 * - Loop control:
 *   - Loop=true: Resets after all items emitted, continues indefinitely
 *   - Loop=false: Stops emitting after each item has been emitted exactly once
 *   - Works for both sequential and random modes (tracks emitted items)
 * 
 * Replaces: RandomWordSourceMod, FileTextSourceMod
 */
class TextSourceMod : public Mod {
public:
  TextSourceMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config,
                const std::string& textSourcesPath);
  ~TextSourceMod();
  float getAgency() const override;
  
  void receive(int sinkId, const float& value) override;
  void applyIntent(const Intent& intent, float strength) override;
  
  static constexpr int SOURCE_TEXT = 1;
  static constexpr int SINK_NEXT = 1;
  
protected:
  void initParameters() override;
  
private:
  // State
  std::filesystem::path textSourcesPath;
  std::vector<std::string> items;  // Generic: words or lines
  bool hasLoadedFile { false };
  
  // Emission tracking (for non-looping exhaustion)
  std::unordered_set<int> emittedIndices;  // Indices already emitted
  int nextSequentialIndex { 0 };           // Next candidate for sequential pick
  
  // Change detection for listeners (ofParameter fires on any set, not just changes)
  std::string lastLoadedFilename;
  bool lastParseMode { false };
  bool lastLoopValue { true };
  
  // Parameters
  ofParameter<std::string> textFilenameParameter { "TextFilename", "text.txt" };
  ofParameter<bool> parseModeParameter { "ParseWords", false };
  ofParameter<float> randomnessParameter { "Randomness", 0.0, 0.0, 1.0 };
  ofParameter<bool> loopParameter { "Loop", true };
  
  // ParamController for Intent-driven smooth transitions
  ParamController<float> randomnessController { randomnessParameter };
  ofParameter<float> agencyFactorParameter { "AgencyFactor", 1.0, 0.0, 1.0 };
  
  // Methods
  void loadFile();
  void resetEmissionState();  // Clear emitted tracking, called on load and loop toggle
  void onTextFilenameChanged(std::string& filename);
  void onParseModeChanged(bool& parseWords);
  void onLoopChanged(bool& loop);
  void emitNext();
};

} // namespace ofxMarkSynth
