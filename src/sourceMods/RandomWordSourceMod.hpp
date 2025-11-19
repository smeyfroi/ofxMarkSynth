//
//  RandomWordSourceMod.hpp
//  ofxMarkSynth
//
//  Created by Steve Meyfroidt on 18/11/2025.
//

#pragma once

#include "Mod.hpp"
#include "ParamController.h"
#include <vector>



namespace ofxMarkSynth {



class RandomWordSourceMod : public Mod {

public:
  RandomWordSourceMod(Synth* synthPtr, const std::string& name, ModConfig config,
                      const std::string& filePath);
  
  void receive(int sinkId, const float& value) override;
  
  static constexpr int SOURCE_TEXT = 1;
  static constexpr int SINK_NEXT_WORD = 1;
  
protected:
  void initParameters() override;
  
private:
  std::vector<std::string> words;
  bool hasLoadedFile { false };
  
  void loadFile(const std::string& filePath);
  void emitNextWord();
};



} // ofxMarkSynth
