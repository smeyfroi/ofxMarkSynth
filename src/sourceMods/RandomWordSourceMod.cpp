//
//  RandomWordSourceMod.cpp
//  ofxMarkSynth
//
//  Created by Steve Meyfroidt on 18/11/2025.
//

#include "RandomWordSourceMod.hpp"
#include "Synth.hpp"
#include "ofUtils.h"



namespace ofxMarkSynth {



RandomWordSourceMod::RandomWordSourceMod(Synth* synthPtr, const std::string& name, ModConfig config,
                                         const std::string& filePath)
: Mod { synthPtr, name, std::move(config) }
{
  sourceNameIdMap = {
    { "text", SOURCE_TEXT }
  };
  
  sinkNameIdMap = {
    { "nextWord", SINK_NEXT_WORD }
  };
  
  loadFile(filePath);
}

void RandomWordSourceMod::initParameters() {
}

void RandomWordSourceMod::loadFile(const std::string& filePath) {
  ofBuffer buffer = ofBufferFromFile(filePath);
  if (!buffer.size()) {
    ofLogError("RandomWordSourceMod") << "Failed to load file: " << filePath;
    hasLoadedFile = false;
    return;
  }
  
  words.clear();
  
  for (const auto& line : buffer.getLines()) {
    std::string trimmedLine = ofTrimFront(ofTrimBack(line));
    if (trimmedLine.empty()) continue;
    
    auto wordTokens = ofSplitString(trimmedLine, " ", true, true);
    for (const auto& word : wordTokens) {
      if (!word.empty()) {
        words.push_back(word);
      }
    }
  }
  
  hasLoadedFile = true;
  ofLogNotice("RandomWordSourceMod") << "Loaded " << words.size() << " words from " << filePath;
}

void RandomWordSourceMod::receive(int sinkId, const float& value) {
  if (sinkId == SINK_NEXT_WORD && value > 0.0f) {
    emitNextWord();
  }
}

void RandomWordSourceMod::emitNextWord() {
  if (!hasLoadedFile || words.empty()) {
    return;
  }
  
  int wordIndex = ofRandom(words.size());
  emit(SOURCE_TEXT, words[wordIndex]);
//  ofLogVerbose("RandomWordSourceMod") << "Emitted word: '" << words[wordIndex] << "'";
}



} // ofxMarkSynth
