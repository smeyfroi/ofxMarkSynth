//
//  TextSourceMod.cpp
//  ofxMarkSynth
//

#include "TextSourceMod.hpp"
#include "core/Synth.hpp"
#include "core/IntentMapper.hpp"
#include "ofUtils.h"

namespace ofxMarkSynth {

TextSourceMod::TextSourceMod(std::shared_ptr<Synth> synthPtr, const std::string& name, 
                             ModConfig config, const std::string& textSourcesPath_)
: Mod { synthPtr, name, std::move(config) },
  textSourcesPath { textSourcesPath_ }
{
  sourceNameIdMap = {
    { "Text", SOURCE_TEXT }
  };
  
  sinkNameIdMap = {
    { "Next", SINK_NEXT }
  };
  
  // Expose controller so UI can show contribution weights
  registerControllerForSource(randomnessParameter, randomnessController);
}


TextSourceMod::~TextSourceMod() {
  // Clean up listeners to prevent memory leaks on mod destruction
  textFilenameParameter.removeListener(this, &TextSourceMod::onTextFilenameChanged);
  parseModeParameter.removeListener(this, &TextSourceMod::onParseModeChanged);
  loopParameter.removeListener(this, &TextSourceMod::onLoopChanged);
}

float TextSourceMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

void TextSourceMod::initParameters() {
  // Add all parameters to GUI
  parameters.add(textFilenameParameter);
  parameters.add(parseModeParameter);
  parameters.add(randomnessParameter);
  parameters.add(loopParameter);
  parameters.add(agencyFactorParameter);
  
  // Register listeners for file/mode changes
  textFilenameParameter.addListener(this, &TextSourceMod::onTextFilenameChanged);
  parseModeParameter.addListener(this, &TextSourceMod::onParseModeChanged);
  loopParameter.addListener(this, &TextSourceMod::onLoopChanged);
  
  // Initialize ParamController for Intent integration
  randomnessController.update();
  
  // Load initial file
  loadFile();
}

void TextSourceMod::loadFile() {
  std::filesystem::path fullPath = textSourcesPath / textFilenameParameter.get();
  
  ofBuffer buffer = ofBufferFromFile(fullPath.string());
  if (!buffer.size()) {
    ofLogError("TextSourceMod") << "Failed to load file: " << fullPath;
    hasLoadedFile = false;
    items.clear();
    return;
  }
  
  items.clear();
  bool parseWords = parseModeParameter.get();
  
  for (const auto& line : buffer.getLines()) {
    std::string trimmedLine = ofTrimFront(ofTrimBack(line));
    if (trimmedLine.empty()) continue;
    
    if (parseWords) {
      // Word mode: split line into individual words
      auto wordTokens = ofSplitString(trimmedLine, " ", true, true);
      for (const auto& word : wordTokens) {
        if (!word.empty()) {
          items.push_back(word);
        }
      }
    } else {
      // Line mode: keep whole line intact
      items.push_back(trimmedLine);
    }
  }
  
  hasLoadedFile = true;
  resetEmissionState();
  
  // Update tracking variables for change detection in listeners
  lastLoadedFilename = textFilenameParameter.get();
  lastParseMode = parseWords;
  lastLoopValue = loopParameter.get();
  
  std::string modeStr = parseWords ? "words" : "lines";
  ofLogNotice("TextSourceMod") << "Loaded " << items.size() 
                               << " " << modeStr << " from " << fullPath;
}

void TextSourceMod::resetEmissionState() {
  emittedIndices.clear();
  nextSequentialIndex = 0;
}

void TextSourceMod::onTextFilenameChanged(std::string& filename) {
  // Only reload if filename actually changed (listeners fire on any assignment)
  if (filename == lastLoadedFilename) {
    return;
  }
  ofLogNotice("TextSourceMod") << "Text filename changed to: " << filename;
  loadFile();  // loadFile() calls resetEmissionState()
}

void TextSourceMod::onParseModeChanged(bool& parseWords) {
  // Only reload if parse mode actually changed (listeners fire on any assignment)
  if (parseWords == lastParseMode) {
    return;
  }
  std::string mode = parseWords ? "words" : "lines";
  ofLogNotice("TextSourceMod") << "Parse mode changed to: " << mode;
  loadFile();  // loadFile() calls resetEmissionState()
}

void TextSourceMod::onLoopChanged(bool& loop) {
  // Only act if loop actually changed to true (listeners fire on any assignment)
  if (loop && !lastLoopValue) {
    // User toggled loop from false to true - reset emission state to allow emitting again
    resetEmissionState();
    ofLogNotice("TextSourceMod") << "Loop enabled, reset emission state";
  }
  lastLoopValue = loop;
}

void TextSourceMod::emitNext() {
  if (!hasLoadedFile || items.empty()) {
    return;
  }
  
  // Check if exhausted (all items emitted and not looping)
  if (emittedIndices.size() >= items.size()) {
    if (loopParameter) {
      resetEmissionState();
    } else {
      return;  // Exhausted and not looping - stop emitting
    }
  }
  
  // Get current randomness value (potentially influenced by Intent)
  float randomness = randomnessController.value;
  
  int index;
  
  // Probabilistic selection based on randomness parameter
  // randomness = 0.0 → always sequential (next unvisited item in order)
  // randomness = 0.5 → 50% random, 50% sequential picks from remaining items
  // randomness = 1.0 → always random (any unvisited item)
  if (ofRandom(1.0f) < randomness) {
    // Random selection: pick any unvisited item using rejection sampling
    // Efficient when <50% emitted; bounded by item count near exhaustion
    do {
      index = static_cast<int>(ofRandom(items.size()));
    } while (emittedIndices.count(index) > 0);
  } else {
    // Sequential selection: find next unvisited item starting from nextSequentialIndex
    while (emittedIndices.count(nextSequentialIndex) > 0) {
      nextSequentialIndex++;
      if (nextSequentialIndex >= static_cast<int>(items.size())) {
        nextSequentialIndex = 0;  // Wrap around to find remaining items
      }
    }
    index = nextSequentialIndex;
    nextSequentialIndex++;
    if (nextSequentialIndex >= static_cast<int>(items.size())) {
      nextSequentialIndex = 0;
    }
  }
  
  emittedIndices.insert(index);
  emit(SOURCE_TEXT, items[index]);
  
  ofLogVerbose("TextSourceMod") << "Emitted item[" << index << "]: '" 
                                << items[index] << "' (randomness=" 
                                << randomness << ", remaining=" 
                                << (items.size() - emittedIndices.size()) << ")";
}

void TextSourceMod::receive(int sinkId, const float& value) {
  if (sinkId == SINK_NEXT && value > 0.0f) {
    emitNext();
  }
}

void TextSourceMod::applyIntent(const Intent& intent, float strength) {
  IntentMap im(intent);
  
  im.C().lin(randomnessController, strength);
  randomnessController.update();
}

} // namespace ofxMarkSynth
