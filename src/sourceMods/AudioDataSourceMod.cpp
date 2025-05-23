//
//  AudioDataSourceMod.cpp
//  example_audio
//
//  Created by Steve Meyfroidt on 03/05/2025.
//

#include "AudioDataSourceMod.hpp"


namespace ofxMarkSynth {


AudioDataSourceMod::AudioDataSourceMod(const std::string& name, const ModConfig&& config, std::shared_ptr<ofxAudioData::Processor> audioDataProcessorPtr_)
: Mod { name, std::move(config) },
  audioDataProcessorPtr { audioDataProcessorPtr_ }
{}

void AudioDataSourceMod::initParameters() {
  parameters.add(minPitchParameter);
  parameters.add(maxPitchParameter);
  parameters.add(minRmsParameter);
  parameters.add(maxRmsParameter);
  parameters.add(minComplexSpectralDifferenceParameter);
  parameters.add(maxComplexSpectralDifferenceParameter);
  parameters.add(minSpectralCrestParameter);
  parameters.add(maxSpectralCrestParameter);
  parameters.add(minZeroCrossingRateParameter);
  parameters.add(maxZeroCrossingRateParameter);
}

float AudioDataSourceMod::getAnalysisScalar(float minParameter, float maxParameter, ofxAudioAnalysisClient::AnalysisScalar scalar) {
  if (minParameter == 0.0 && maxParameter == 0.0) {
    return audioDataProcessorPtr->getNormalisedScalarValue(scalar);
  } else {
    return audioDataProcessorPtr->getNormalisedScalarValue(scalar, minParameter, maxParameter);
  }
}

void AudioDataSourceMod::emitPitchRmsPoints() {
  float x = getAnalysisScalar(minPitchParameter,
                              maxPitchParameter,
                              ofxAudioAnalysisClient::AnalysisScalar::pitch);
  float y = getAnalysisScalar(minRmsParameter,
                              maxRmsParameter,
                              ofxAudioAnalysisClient::AnalysisScalar::rootMeanSquare);
  emit(SOURCE_PITCH_RMS_POINTS, glm::vec2 { x, y });
}

void AudioDataSourceMod::emitSpectralPoints() {
  float x = getAnalysisScalar(minComplexSpectralDifferenceParameter,
                              maxComplexSpectralDifferenceParameter,
                              ofxAudioAnalysisClient::AnalysisScalar::complexSpectralDifference);
  float y = getAnalysisScalar(minSpectralCrestParameter,
                              maxSpectralCrestParameter,
                              ofxAudioAnalysisClient::AnalysisScalar::spectralCrest);
  float z = getAnalysisScalar(minZeroCrossingRateParameter,
                              maxZeroCrossingRateParameter,
                              ofxAudioAnalysisClient::AnalysisScalar::zeroCrossingRate);
  emit(SOURCE_SPECTRAL_POINTS, glm::vec3 { x, y, z });
}

void AudioDataSourceMod::emitScalar(int sourceId, float minParameter, float maxParameter, ofxAudioAnalysisClient::AnalysisScalar scalar) {
  float s = getAnalysisScalar(minComplexSpectralDifferenceParameter,
                              maxComplexSpectralDifferenceParameter,
                              scalar);
  emit(sourceId, s);
}

void AudioDataSourceMod::update() {
  if (!audioDataProcessorPtr) { ofLogError() << "update in " << typeid(*this).name() << " with no audioDataProcessor"; return; }
  if (audioDataProcessorPtr->isDataValid()) {
    if (connections.contains(SOURCE_PITCH_RMS_POINTS)) emitPitchRmsPoints();
    if (connections.contains(SOURCE_SPECTRAL_POINTS)) emitSpectralPoints();
    if (connections.contains(SOURCE_PITCH_SCALAR)) emitScalar(SOURCE_PITCH_SCALAR, minPitchParameter, maxPitchParameter, ofxAudioAnalysisClient::AnalysisScalar::pitch);
    if (connections.contains(SOURCE_RMS_SCALAR)) emitScalar(SOURCE_RMS_SCALAR, minRmsParameter, maxRmsParameter, ofxAudioAnalysisClient::AnalysisScalar::rootMeanSquare);
    if (connections.contains(SOURCE_COMPLEX_SPECTRAL_DIFFERENCE_SCALAR)) emitScalar(SOURCE_COMPLEX_SPECTRAL_DIFFERENCE_SCALAR, minComplexSpectralDifferenceParameter, maxComplexSpectralDifferenceParameter, ofxAudioAnalysisClient::AnalysisScalar::complexSpectralDifference);
    if (connections.contains(SOURCE_SPECTRAL_CREST_SCALAR)) emitScalar(SOURCE_SPECTRAL_CREST_SCALAR, minSpectralCrestParameter, maxSpectralCrestParameter, ofxAudioAnalysisClient::AnalysisScalar::spectralCrest);
    if (connections.contains(SOURCE_ZERO_CROSSING_RATE_SCALAR)) emitScalar(SOURCE_ZERO_CROSSING_RATE_SCALAR, minZeroCrossingRateParameter, maxZeroCrossingRateParameter, ofxAudioAnalysisClient::AnalysisScalar::zeroCrossingRate);
  }
}


} // ofxMarkSynth
