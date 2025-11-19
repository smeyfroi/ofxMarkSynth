//
//  FileTextSourceMod.hpp
//  ofxMarkSynth
//
//  Created by Steve Meyfroidt on 18/11/2025.
//

#pragma once

#include "Mod.hpp"
#include "ParamController.h"
#include <vector>



namespace ofxMarkSynth {



class FileTextSourceMod : public Mod {

public:
  FileTextSourceMod(Synth* synthPtr, const std::string& name, ModConfig config,
                    const std::string& filePath);
  
  void receive(int sinkId, const float& value) override;
  
  static constexpr int SOURCE_TEXT = 1;
  static constexpr int SINK_NEXT_LINE = 1;
  
protected:
  void initParameters() override;
  
private:
  std::vector<std::string> lines;
  int currentLineIndex { 0 };
  bool hasLoadedFile { false };
  
  ofParameter<bool> loopParameter { "Loop", true };
  ofParameter<bool> randomOrderParameter { "Random Order", false };
  
  void loadFile(const std::string& filePath);
  void emitNextLine();
};



} // ofxMarkSynth
