//  AudioDataSourceMod.hpp
//  example_audio
//
//  Created by Steve Meyfroidt on 03/05/2025.
//

#pragma once

#include "core/Mod.hpp"
#include "ofxAudioData.h"
#include <memory>

namespace ofxAudioAnalysisClient {
class LocalGistClient;
}

namespace ofxMarkSynth {

class AudioDataSourceMod : public Mod {

public:
  AudioDataSourceMod(std::shared_ptr<Synth> synthPtr,
                     const std::string& name,
                     ModConfig config,
                     std::shared_ptr<ofxAudioAnalysisClient::LocalGistClient> audioAnalysisClient);

  void shutdown() override;
  void update() override;
  void draw() override;
  bool keyPressed(int key) override;

  UiState captureUiState() const override {
    UiState state;
    setUiStateBool(state, "tuningVisible", tuningVisible);
    if (audioDataPlotsPtr) {
      setUiStateBool(state, "plotsVisible", audioDataPlotsPtr->plotsVisible);
    }
    return state;
  }

  void restoreUiState(const UiState& state) override {
    tuningVisible = getUiStateBool(state, "tuningVisible", tuningVisible);
    if (audioDataPlotsPtr) {
      bool defaultPlotsVisible = audioDataPlotsPtr->plotsVisible;
      audioDataPlotsPtr->plotsVisible = getUiStateBool(state, "plotsVisible", defaultPlotsVisible);
    }
  }

  static constexpr int SOURCE_PITCH_RMS_POINTS = 1;
  static constexpr int SOURCE_POLAR_PITCH_RMS_POINTS = 2;
  static constexpr int SOURCE_SPECTRAL_3D_POINTS = 5;
  static constexpr int SOURCE_SPECTRAL_2D_POINTS = 6;
  static constexpr int SOURCE_POLAR_SPECTRAL_2D_POINTS = 7;
  static constexpr int SOURCE_PITCH_SCALAR = 10;
  static constexpr int SOURCE_RMS_SCALAR = 11;
  static constexpr int SOURCE_COMPLEX_SPECTRAL_DIFFERENCE_SCALAR = 12;
  static constexpr int SOURCE_SPECTRAL_CREST_SCALAR = 13;
  static constexpr int SOURCE_ZERO_CROSSING_RATE_SCALAR = 14;
  static constexpr int SOURCE_ONSET1 = 20;
  static constexpr int SOURCE_TIMBRE_CHANGE = 21;
  static constexpr int SOURCE_PITCH_CHANGE = 22;

protected:
  void initParameters() override;

private:
  void initialise();

  std::shared_ptr<ofxAudioAnalysisClient::LocalGistClient> audioAnalysisClientPtr;
  std::shared_ptr<ofxAudioData::Processor> audioDataProcessorPtr;
  std::shared_ptr<ofxAudioData::Plots> audioDataPlotsPtr;
  float lastUpdated = 0.0;

  // https://en.wikipedia.org/wiki/Template:Vocal_and_instrumental_pitch_ranges
  ofParameter<float> minPitchParameter { "MinPitch", 50.0, 0.0, 100.0 };
  ofParameter<float> maxPitchParameter { "MaxPitch", 800.0, 0.0, 3000.0 }; // C8 is ~4400.0 Hz
  ofParameter<float> minRmsParameter { "MinRms", 0.0, 0.0, 0.05 };
  ofParameter<float> maxRmsParameter { "MaxRms", 0.02, 0.005, 0.2 };
  ofParameter<float> minComplexSpectralDifferenceParameter { "MinComplexSpectralDifference", 20.0, 0.0, 2000.0 };
  ofParameter<float> maxComplexSpectralDifferenceParameter { "MaxComplexSpectralDifference", 70.0, 0.0, 2000.0 };
  ofParameter<float> minSpectralCrestParameter { "MinSpectralCrest", 20.0, 0.0, 500.0 };
  ofParameter<float> maxSpectralCrestParameter { "MaxSpectralCrest", 100.0, 0.0, 500.0 };
  ofParameter<float> minZeroCrossingRateParameter { "MinZeroCrossingRate", 5.0, 0.0, 15.0 };
  ofParameter<float> maxZeroCrossingRateParameter { "MaxZeroCrossingRate", 15.0, 2.0, 80.0 };

  float getNormalisedAnalysisScalar(float minParam, float maxParam, ofxAudioAnalysisClient::AnalysisScalar scalar);
  void emitPitchRmsPoints();
  void emitPolarPitchRmsPoints();
  void emitSpectral2DPoints();
  void emitPolarSpectral2DPoints();
  void emitSpectral3DPoints();
  void emitScalar(int sourceId, float minParameter, float maxParameter, ofxAudioAnalysisClient::AnalysisScalar scalar);

  void drawEventDetectionOverlay();

  bool tuningVisible { false };
};

} // ofxMarkSynth
