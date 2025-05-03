//
//  AudioDataSourceMod.hpp
//  example_audio
//
//  Created by Steve Meyfroidt on 03/05/2025.
//

#pragma once

#include "Mod.hpp"
#include "ofxAudioData.h"

namespace ofxMarkSynth {


class AudioDataSourceMod : public Mod {

public:
  AudioDataSourceMod(const std::string& name, const ModConfig&& config);
  void update() override;
  std::shared_ptr<ofxAudioData::Processor> audioDataProcessorPtr;
  
  static constexpr int SOURCE_POINTS = 1;

protected:
  void initParameters() override;

private:
  ofParameter<float> minPitchParameter { "MinPitch", 50.0, 0.0, 6000.0 };
  ofParameter<float> maxPitchParameter { "MaxPitch", 2500.0, 0.0, 6000.0 };
  ofParameter<float> minRmsParameter { "MinRms", 0.001, 0.0, 0.1 };
  ofParameter<float> maxRmsParameter { "MaxRms", 0.03, 0.0, 0.1 };
};


} // ofxMarkSynth
