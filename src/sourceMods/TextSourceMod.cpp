//
//  TextSourceMod.cpp
//  ofxMarkSynth
//

#include "TextSourceMod.hpp"
#include "Synth.hpp"
#include "ofUtils.h"

namespace ofxMarkSynth {

TextSourceMod::TextSourceMod(Synth* synthPtr, const std::string& name, 
                             ModConfig config, const std::string& textSourcesPath_)
: Mod { synthPtr, name, std::move(config) },
  textSourcesPath { textSourcesPath_ }
{
  sourceNameIdMap = {
    { "text", SOURCE_TEXT }
  };
  
  // Register multiple sink names for backward compatibility
  sinkNameIdMap = {
    { "next", SINK_NEXT },          // Primary name
    { "nextWord", SINK_NEXT },      // Backward compat: RandomWordSource
    { "nextLine", SINK_NEXT }       // Backward compat: FileTextSource
  };
}

TextSourceMod::~TextSourceMod() {
  // Clean up listeners to prevent memory leaks on mod destruction
  textFilenameParameter.removeListener(this, &TextSourceMod::onTextFilenameChanged);
  parseModeParameter.removeListener(this, &TextSourceMod::onParseModeChanged);
}

void TextSourceMod::initParameters() {
  // Add all parameters to GUI
  parameters.add(textFilenameParameter);
  parameters.add(parseModeParameter);
  parameters.add(randomnessParameter);
  parameters.add(loopParameter);
  
  // Register listeners for file/mode changes
  textFilenameParameter.addListener(this, &TextSourceMod::onTextFilenameChanged);
  parseModeParameter.addListener(this, &TextSourceMod::onParseModeChanged);
  
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
  currentIndex = 0;  // Reset index when loading new file
  
  std::string modeStr = parseWords ? "words" : "lines";
  ofLogNotice("TextSourceMod") << "Loaded " << items.size() 
                               << " " << modeStr << " from " << fullPath;
}

void TextSourceMod::onTextFilenameChanged(std::string& filename) {
  ofLogNotice("TextSourceMod") << "Text filename changed to: " << filename;
  currentIndex = 0;
  loadFile();
}

void TextSourceMod::onParseModeChanged(bool& parseWords) {
  std::string mode = parseWords ? "words" : "lines";
  ofLogNotice("TextSourceMod") << "Parse mode changed to: " << mode;
  currentIndex = 0;  // Reset index when changing mode
  loadFile();
}

void TextSourceMod::emitNext() {
  if (!hasLoadedFile || items.empty()) {
    return;
  }
  
  // Get current randomness value (potentially influenced by Intent)
  float randomness = randomnessController.value;
  
  int index;
  
  // Probabilistic selection based on randomness parameter
  // randomness = 0.0 → always sequential
  // randomness = 0.5 → 50% random, 50% sequential
  // randomness = 1.0 → always random
  if (ofRandom(1.0f) < randomness) {
    // Random selection: pick any item
    index = ofRandom(items.size());
  } else {
    // Sequential selection: use current index and advance
    index = currentIndex;
    
    currentIndex++;
    if (currentIndex >= items.size()) {
      if (loopParameter) {
        currentIndex = 0;  // Loop back to start
      } else {
        currentIndex = items.size() - 1;  // Stay on last item
      }
    }
  }
  
  emit(SOURCE_TEXT, items[index]);
  ofLogVerbose("TextSourceMod") << "Emitted item[" << index << "]: '" 
                                << items[index] << "' (randomness=" 
                                << randomness << ")";
}

void TextSourceMod::receive(int sinkId, const float& value) {
  if (sinkId == SINK_NEXT && value > 0.0f) {
    emitNext();
  }
}

void TextSourceMod::applyIntent(const Intent& intent, float strength) {
  if (strength < 0.01) return;
  
  // Map Chaos dimension (0.0-1.0) directly to randomness (0.0-1.0)
  // Low chaos → sequential, structured, predictable
  // High chaos → random, chaotic, unpredictable
  float chaos = intent.getChaos();
  
  // Use ParamController for smooth transitions between Intent states
  randomnessController.updateIntent(chaos, strength);
  randomnessController.update();
}

} // namespace ofxMarkSynth
