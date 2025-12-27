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
#include <filesystem>

namespace ofxMarkSynth {

/**
 * TextSourceMod - Unified text source with flexible selection strategies
 * 
 * Features:
 * - Parse mode: Split into words OR keep whole lines
 * - Randomness: Float parameter (0.0-1.0) controls selection probability
 *   - 0.0 = Pure sequential (structured, predictable)
 *   - 0.5 = Mixed (50% random, 50% sequential)
 *   - 1.0 = Pure random (chaotic, unpredictable)
 * - Intent integration: Chaos dimension maps to randomness
 * - Loop control for sequential mode
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
  int currentIndex { 0 };
  bool hasLoadedFile { false };
  
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
  void onTextFilenameChanged(std::string& filename);
  void onParseModeChanged(bool& parseWords);
  void emitNext();
};

} // namespace ofxMarkSynth
