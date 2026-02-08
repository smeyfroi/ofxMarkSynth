//  AudioDataSourceMod.hpp
//  example_audio
//
//  Created by Steve Meyfroidt on 03/05/2025.
//

#pragma once

#include "core/Mod.hpp"
#include "ofxAudioData.h"
#include <cstdint>
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

  std::shared_ptr<ofxAudioData::Processor> getAudioDataProcessor() const { return audioDataProcessorPtr; }

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
  static constexpr int SOURCE_DRIFT_PITCH_RMS_POINTS = 3;
  static constexpr int SOURCE_SPECTRAL_3D_POINTS = 5;
  static constexpr int SOURCE_SPECTRAL_2D_POINTS = 6;
  static constexpr int SOURCE_POLAR_SPECTRAL_2D_POINTS = 7;
  static constexpr int SOURCE_DRIFT_SPECTRAL_2D_POINTS = 8;
  static constexpr int SOURCE_PITCH_SCALAR = 10;
  static constexpr int SOURCE_RMS_SCALAR = 11;
  static constexpr int SOURCE_SPECTRAL_CENTROID_SCALAR = 12;
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

  // Scalar filter selection (0 = faster/less smooth, 1 = smoother)
  ofParameter<int> scalarFilterIndexParameter { "ScalarFilterIndex", 1, 0, ofxAudioData::Processor::filterCount - 1 };

  // https://en.wikipedia.org/wiki/Template:Vocal_and_instrumental_pitch_ranges
  // Baseline defaults (frozen): tuned against prerecorded WAV baseline.
  // Ranges remain intentionally wide so we can re-tune later if needed.
  ofParameter<float> minPitchParameter { "MinPitch", 60.0, 0.0, 2000.0 };
  ofParameter<float> maxPitchParameter { "MaxPitch", 440.0, 0.0, 6000.0 }; // C8 is ~4400.0 Hz
  ofParameter<float> minRmsParameter { "MinRms", 0.01, 0.0, 1.0 };
  ofParameter<float> maxRmsParameter { "MaxRms", 0.14, 0.0, 1.0 };

  // Spectral centroid (Gist returns this as a bin index, not Hz).
  ofParameter<float> minSpectralCentroidParameter { "MinSpectralCentroid", 25.0, 0.0, 100.0 };
  ofParameter<float> maxSpectralCentroidParameter { "MaxSpectralCentroid", 35.0, 0.0, 100.0 };

  ofParameter<float> minSpectralCrestParameter { "MinSpectralCrest", 90.0, 0.0, 5000.0 };
  ofParameter<float> maxSpectralCrestParameter { "MaxSpectralCrest", 170.0, 0.0, 5000.0 };
  ofParameter<float> minZeroCrossingRateParameter { "MinZeroCrossingRate", 12.0, 0.0, 500.0 };
  ofParameter<float> maxZeroCrossingRateParameter { "MaxZeroCrossingRate", 28.0, 0.0, 500.0 };

  // Drift mapping: keep quiet passages off the top edge.
  // When raw y is quiet (follow ~ 0), we mostly hold + drift; when loud (follow ~ 1), we track.
  ofParameter<float> driftFollowMinParameter { "DriftFollowMin", 0.05, 0.0, 1.0 };
  ofParameter<float> driftFollowMaxParameter { "DriftFollowMax", 0.25, 0.0, 1.0 };
  ofParameter<float> driftFollowGammaParameter { "DriftFollowGamma", 1.2, 0.1, 8.0 };
  ofParameter<float> driftAccelParameter { "DriftAccel", 0.004, 0.0, 0.05 };
  ofParameter<float> driftDampingParameter { "DriftDamping", 0.92, 0.0, 0.999 };
  ofParameter<float> driftCenterSpringParameter { "DriftCenterSpring", 0.02, 0.0, 0.2 };
  ofParameter<float> driftMaxVelocityParameter { "DriftMaxVelocity", 0.02, 0.0, 0.2 };

  float getNormalisedAnalysisScalar(float minParam, float maxParam, ofxAudioAnalysisClient::AnalysisScalar scalar);
  void emitPitchRmsPoints();
  void emitPolarPitchRmsPoints();
  void emitDriftPitchRmsPoints();
  void emitSpectral2DPoints();
  void emitDriftSpectral2DPoints();
  void emitPolarSpectral2DPoints();
  void emitSpectral3DPoints();
  void emitScalar(int sourceId, float minParameter, float maxParameter, ofxAudioAnalysisClient::AnalysisScalar scalar);

  float updateDriftY(float yRaw, float& yState, float& yVelocity);
  float getRandomSignedFloat();

  void drawEventDetectionOverlay();

  float driftPitchRmsYState { 0.5f };
  float driftPitchRmsYVelocity { 0.0f };
  float driftSpectral2DYState { 0.5f };
  float driftSpectral2DYVelocity { 0.0f };
  uint32_t driftRngState { 0x9E3779B9u };

  bool tuningVisible { false };
};

} // ofxMarkSynth
