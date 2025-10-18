//
//  AudioDataSourceMod.cpp
//  example_audio
//
//  Created by Steve Meyfroidt on 03/05/2025.
//

#include "AudioDataSourceMod.hpp"
#include <cmath>


namespace ofxMarkSynth {


AudioDataSourceMod::AudioDataSourceMod(const std::string& name, const ModConfig&& config, const std::string& micDeviceName, bool recordAudio, const std::filesystem::path& recordingPath, const std::filesystem::path& rootSourceMaterialPath)
: Mod { name, std::move(config) }
{
  std::filesystem::create_directory(recordingPath);

//  audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>(micDeviceName, recordAudio, recordingPath);
  
//  audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>(rootSourceMaterialPath/"belfast/20250208-violin-separate-scale-vibrato-harmonics.wav");
//  audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>(rootSourceMaterialPath/"percussion/Alex Petcu Bell Plates.wav");
//        audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>(rootSourceMaterialPath/"percussion/Alex Petcu Sound Bath.wav");
  audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>(rootSourceMaterialPath/"belfast/20250208-trombone-melody.wav");
//  audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>(rootSourceMaterialPath/"misc/nightsong.wav");
//  audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>(rootSourceMaterialPath/"misc/treganna.wav");

  audioDataProcessorPtr = std::make_shared<ofxAudioData::Processor>(audioAnalysisClientPtr);
  audioDataProcessorPtr->setDefaultValiditySpecs();
  audioDataPlotsPtr = std::make_shared<ofxAudioData::Plots>(audioDataProcessorPtr);
}

void AudioDataSourceMod::registerAudioCallback(ofxAudioAnalysisClient::LocalGistClient::AudioFrameCallback audioFrameCallback) {
  audioAnalysisClientPtr->setAudioFrameCallback(audioFrameCallback);
}

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

float AudioDataSourceMod::getNormalisedAnalysisScalar(float minParameter, float maxParameter, ofxAudioAnalysisClient::AnalysisScalar scalar) {
//  if (minParameter == 0.0 && maxParameter == 0.0) {
//    return audioDataProcessorPtr->getNormalisedScalarValue(scalar);
//  } else {
    return audioDataProcessorPtr->getNormalisedScalarValue(scalar, minParameter, maxParameter, true);
//  }
}

void AudioDataSourceMod::emitPitchRmsPoints() {
  float x = getNormalisedAnalysisScalar(minPitchParameter, maxPitchParameter,
                                        ofxAudioAnalysisClient::AnalysisScalar::pitch);
  float y = getNormalisedAnalysisScalar(minRmsParameter, maxRmsParameter,
                                        ofxAudioAnalysisClient::AnalysisScalar::rootMeanSquare);
  emit(SOURCE_PITCH_RMS_POINTS, glm::vec2 { x, y });
}

void AudioDataSourceMod::emitPolarPitchRmsPoints() {
  // Note that pitch is wrapped around within the min/max range
//  float pitch = audioDataProcessorPtr->getScalarValue(ofxAudioAnalysisClient::AnalysisScalar::pitch);
  float pitch = getNormalisedAnalysisScalar(minPitchParameter, maxPitchParameter,
                                            ofxAudioAnalysisClient::AnalysisScalar::pitch);
  // 0.7 to get into the corners
  float rms = 0.7 * getNormalisedAnalysisScalar(minRmsParameter, maxRmsParameter,
                                          ofxAudioAnalysisClient::AnalysisScalar::rootMeanSquare);
  // map pitch to two circumferences to avoid bunching
  float angle = std::fmod(2 * pitch * glm::two_pi<float>(), glm::two_pi<float>());
  float x = rms * std::cos(angle);
  float y = rms * std::sin(angle);
  x += 0.5; y += 0.5;
  if (x < 0.0) x += 1.0;
  if (x > 1.0) x -= 1.0;
  if (y < 0.0) y += 1.0;
  if (y > 1.0) y -= 1.0;
  emit(SOURCE_POLAR_PITCH_RMS_POINTS, glm::vec2 { x, y });
}

void AudioDataSourceMod::emitSpectralPoints() {
  float x = getNormalisedAnalysisScalar(minComplexSpectralDifferenceParameter,
                              maxComplexSpectralDifferenceParameter,
                              ofxAudioAnalysisClient::AnalysisScalar::complexSpectralDifference);
  float y = getNormalisedAnalysisScalar(minSpectralCrestParameter,
                              maxSpectralCrestParameter,
                              ofxAudioAnalysisClient::AnalysisScalar::spectralCrest);
  float z = getNormalisedAnalysisScalar(minZeroCrossingRateParameter,
                              maxZeroCrossingRateParameter,
                              ofxAudioAnalysisClient::AnalysisScalar::zeroCrossingRate);
  emit(SOURCE_SPECTRAL_POINTS, glm::vec3 { x, y, z });
}

void AudioDataSourceMod::emitScalar(int sourceId, float minParameter, float maxParameter, ofxAudioAnalysisClient::AnalysisScalar scalar) {
  float s = getNormalisedAnalysisScalar(minParameter, maxParameter, scalar);
  emit(sourceId, s);
}

void AudioDataSourceMod::update() {
  if (!audioDataProcessorPtr) { ofLogError() << "update in " << typeid(*this).name() << " with no audioDataProcessor"; return; }
  audioDataProcessorPtr->update();
  
  if (!audioDataProcessorPtr->isDataUpdated(lastUpdated)) return;
  
  lastUpdated = audioDataProcessorPtr->getLastUpdateTimestamp();
  
  for (const auto& [sourceId, sinks] : connections) {
    switch (sourceId) {
      case SOURCE_PITCH_RMS_POINTS:
        emitPitchRmsPoints();
        break;
      case SOURCE_POLAR_PITCH_RMS_POINTS:
        emitPolarPitchRmsPoints();
        break;
      case SOURCE_SPECTRAL_POINTS:
        emitSpectralPoints();
        break;
      case SOURCE_PITCH_SCALAR:
        emitScalar(SOURCE_PITCH_SCALAR, minPitchParameter, maxPitchParameter,
                   ofxAudioAnalysisClient::AnalysisScalar::pitch);
        break;
      case SOURCE_RMS_SCALAR:
        emitScalar(SOURCE_RMS_SCALAR, minRmsParameter, maxRmsParameter,
                   ofxAudioAnalysisClient::AnalysisScalar::rootMeanSquare);
        break;
      case SOURCE_COMPLEX_SPECTRAL_DIFFERENCE_SCALAR:
        emitScalar(SOURCE_COMPLEX_SPECTRAL_DIFFERENCE_SCALAR, minComplexSpectralDifferenceParameter,
                   maxComplexSpectralDifferenceParameter,
                   ofxAudioAnalysisClient::AnalysisScalar::complexSpectralDifference);
        break;
      case SOURCE_SPECTRAL_CREST_SCALAR:
        emitScalar(SOURCE_SPECTRAL_CREST_SCALAR, minSpectralCrestParameter, maxSpectralCrestParameter,
                   ofxAudioAnalysisClient::AnalysisScalar::spectralCrest);
        break;
      case SOURCE_ZERO_CROSSING_RATE_SCALAR:
        emitScalar(SOURCE_ZERO_CROSSING_RATE_SCALAR, minZeroCrossingRateParameter, maxZeroCrossingRateParameter,
                   ofxAudioAnalysisClient::AnalysisScalar::zeroCrossingRate);
        break;
      default:
        break;
    }
  }
  
  float onsetMagnitude = audioDataProcessorPtr->detectOnset1();
  if (onsetMagnitude > 0.0f) emit(SOURCE_ONSET1, onsetMagnitude);
  float timbreChangeMagnitude = audioDataProcessorPtr->detectTimbreChange1();
  if (timbreChangeMagnitude > 0.0f) emit(SOURCE_TIMBRE_CHANGE, timbreChangeMagnitude);
}

bool AudioDataSourceMod::keyPressed(int key) {
  if (audioAnalysisClientPtr->keyPressed(key)) return true;
  if (audioDataPlotsPtr->keyPressed(key)) return true;
  return false;
}

void AudioDataSourceMod::draw() {
  ofPushMatrix();
  audioDataPlotsPtr->drawPlots();
  ofPopMatrix();
}

void AudioDataSourceMod::shutdown() {
  audioAnalysisClientPtr->stopRecording();
  audioAnalysisClientPtr->closeStream();
}


} // ofxMarkSynth
