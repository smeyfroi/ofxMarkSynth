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
  AudioDataSourceMod(const std::string& name, const ModConfig&& config, std::shared_ptr<ofxAudioData::Processor> audioDataProcessorPtr);
  void update() override;
  
  static constexpr int SOURCE_PITCH_RMS_POINTS = 1;
  static constexpr int SOURCE_POLAR_PITCH_RMS_POINTS = 2;
  static constexpr int SOURCE_SPECTRAL_POINTS = 5;
  static constexpr int SOURCE_PITCH_SCALAR = 10;
  static constexpr int SOURCE_RMS_SCALAR = 11;
  static constexpr int SOURCE_COMPLEX_SPECTRAL_DIFFERENCE_SCALAR = 12;
  static constexpr int SOURCE_SPECTRAL_CREST_SCALAR = 13;
  static constexpr int SOURCE_ZERO_CROSSING_RATE_SCALAR = 14;

protected:
  void initParameters() override;

private:
  std::shared_ptr<ofxAudioData::Processor> audioDataProcessorPtr;
  float lastUpdated = 0.0;
  
  ofParameter<float> minPitchParameter { "MinPitch", 50.0, 0.0, 6000.0 };
  ofParameter<float> maxPitchParameter { "MaxPitch", 2500.0, 0.0, 6000.0 };
  ofParameter<float> minRmsParameter { "MinRms", 0.005, 0.0, 0.1 };
  ofParameter<float> maxRmsParameter { "MaxRms", 0.05, 0.0, 0.2 };
  ofParameter<float> minComplexSpectralDifferenceParameter { "MinComplexSpectralDifference", 20.0, 0.0, 120.0 };
  ofParameter<float> maxComplexSpectralDifferenceParameter { "MaxComplexSpectralDifference", 70.0, 0.0, 120.0 };
  ofParameter<float> minSpectralCrestParameter { "MinSpectralCrest", 60.0, 0.0, 200.0 };
  ofParameter<float> maxSpectralCrestParameter { "MaxSpectralCrest", 150.0, 0.0, 200.0 };
  ofParameter<float> minZeroCrossingRateParameter { "MinZeroCrossingRate", 10.0, 0.0, 100.0 };
  ofParameter<float> maxZeroCrossingRateParameter { "MaxZeroCrossingRate", 30.0, 0.0, 100.0 };
  
  float getNormalisedAnalysisScalar(float minParam, float maxParam, ofxAudioAnalysisClient::AnalysisScalar scalar);
  void emitPitchRmsPoints();
  void emitPolarPitchRmsPoints();
  void emitSpectralPoints();
  void emitScalar(int sourceId, float minParameter, float maxParameter, ofxAudioAnalysisClient::AnalysisScalar scalar);

};


} // ofxMarkSynth
