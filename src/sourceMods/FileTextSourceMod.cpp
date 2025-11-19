//
//  FileTextSourceMod.cpp
//  ofxMarkSynth
//
//  Created by Steve Meyfroidt on 18/11/2025.
//

#include "FileTextSourceMod.hpp"
#include "Synth.hpp"
#include "ofUtils.h"



namespace ofxMarkSynth {



FileTextSourceMod::FileTextSourceMod(Synth* synthPtr, const std::string& name, ModConfig config,
                                     const std::string& filePath)
: Mod { synthPtr, name, std::move(config) }
{
  sourceNameIdMap = {
    { "text", SOURCE_TEXT }
  };
  
  sinkNameIdMap = {
    { "nextLine", SINK_NEXT_LINE }
  };
  
  loadFile(filePath);
}

void FileTextSourceMod::initParameters() {
  parameters.add(loopParameter);
  parameters.add(randomOrderParameter);
}

void FileTextSourceMod::loadFile(const std::string& filePath) {
  ofBuffer buffer = ofBufferFromFile(filePath);
  
  if (!buffer.size()) {
    ofLogError("FileTextSourceMod") << "Failed to load file: " << filePath;
    hasLoadedFile = false;
    return;
  }
  
  lines.clear();
  for (const auto& line : buffer.getLines()) {
    std::string trimmed = ofTrimFront(ofTrimBack(line));
    if (!trimmed.empty()) {
      lines.push_back(trimmed);
    }
  }
  
  hasLoadedFile = true;
  ofLogNotice("FileTextSourceMod") << "Loaded " << lines.size() << " lines from " << filePath;
}

void FileTextSourceMod::receive(int sinkId, const float& value) {
  if (sinkId == SINK_NEXT_LINE && value > 0.0f) {
    emitNextLine();
  }
}

void FileTextSourceMod::emitNextLine() {
  if (!hasLoadedFile || lines.empty()) {
    return;
  }
  
  // Select line
  int lineIndex;
  if (randomOrderParameter) {
    lineIndex = ofRandom(lines.size());
  } else {
    lineIndex = currentLineIndex;
  }
  
  // Emit the line
  emit(SOURCE_TEXT, lines[lineIndex]);
  ofLogNotice("FileTextSourceMod") << "Emitted line " << lineIndex << ": '" << lines[lineIndex] << "'";
  
  // Advance index for sequential mode
  if (!randomOrderParameter) {
    currentLineIndex++;
    if (currentLineIndex >= lines.size()) {
      if (loopParameter) {
        currentLineIndex = 0;
      } else {
        currentLineIndex = lines.size() - 1; // Stay on last line
      }
    }
  }
}



} // ofxMarkSynth
