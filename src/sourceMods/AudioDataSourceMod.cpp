//
//  AudioDataSourceMod.cpp
//  example_audio
//
//  Created by Steve Meyfroidt on 03/05/2025.
//

#include "AudioDataSourceMod.hpp"
#include <cmath>


namespace ofxMarkSynth {


AudioDataSourceMod::AudioDataSourceMod(Synth* synthPtr, const std::string& name, const ModConfig&& config, const std::string& micDeviceName, bool recordAudio, const std::filesystem::path& recordingPath, const std::filesystem::path& rootSourceMaterialPath)
: Mod { synthPtr, name, std::move(config) },
tuningVisible(false)
{
  std::filesystem::create_directory(recordingPath);

//  audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>(micDeviceName, recordAudio, recordingPath);
  
  audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>(rootSourceMaterialPath/"belfast/20250208-violin-separate-scale-vibrato-harmonics.wav");
//  audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>(rootSourceMaterialPath/"percussion/Alex Petcu Bell Plates.wav");
//        audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>(rootSourceMaterialPath/"percussion/Alex Petcu Sound Bath.wav");
//  audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>(rootSourceMaterialPath/"belfast/20250208-trombone-melody.wav");
//  audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>(rootSourceMaterialPath/"cork/audio-2025-06-16-11-16-14-782.wav");
//  audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>(rootSourceMaterialPath/"cork/audio-2025-06-16-11-25-03-931.wav");
//  audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>(rootSourceMaterialPath/"misc/nightsong.wav");
//  audioAnalysisClientPtr = std::make_shared<ofxAudioAnalysisClient::LocalGistClient>(rootSourceMaterialPath/"misc/treganna.wav");
  
  audioDataProcessorPtr = std::make_shared<ofxAudioData::Processor>(audioAnalysisClientPtr);
  audioDataProcessorPtr->setDefaultValiditySpecs();
  audioDataPlotsPtr = std::make_shared<ofxAudioData::Plots>(audioDataProcessorPtr);
  
  sourceNameIdMap = {
    { "pitchRmsPoints", SOURCE_PITCH_RMS_POINTS },
    { "polarPitchRmsPoints", SOURCE_POLAR_PITCH_RMS_POINTS },
    { "spectral3dPoints", SOURCE_SPECTRAL_3D_POINTS },
    { "spectral2dPoints", SOURCE_SPECTRAL_2D_POINTS },
    { "polarSpectral2dPoints", SOURCE_POLAR_SPECTRAL_2D_POINTS },
    { "pitchScalar", SOURCE_PITCH_SCALAR },
    { "rmsScalar", SOURCE_RMS_SCALAR },
    { "complexSpectralDifferenceScalar", SOURCE_COMPLEX_SPECTRAL_DIFFERENCE_SCALAR },
    { "spectralCrestScalar", SOURCE_SPECTRAL_CREST_SCALAR },
    { "zeroCrossingRateScalar", SOURCE_ZERO_CROSSING_RATE_SCALAR },
    { "onset1", SOURCE_ONSET1 },
    { "timbreChange", SOURCE_TIMBRE_CHANGE },
    { "pitchChange", SOURCE_PITCH_CHANGE }
  };
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
  parameters.add(audioDataProcessorPtr->getParameterGroup());
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

glm::vec2 normalisedAngleLengthToPolar(float angle, float length) {
  length *= 0.7f; // get into the corners
  angle = std::fmod(angle * glm::two_pi<float>() * 2.0f, glm::two_pi<float>()); // map to two circumferences to avoid bunching and wrap
  float x = length * std::cos(angle);
  float y = length * std::sin(angle);
  x += 0.5; y += 0.5;
  if (x < 0.0) x += 1.0;
  if (x > 1.0) x -= 1.0;
  if (y < 0.0) y += 1.0;
  if (y > 1.0) y -= 1.0;
  return { x, y };
}

void AudioDataSourceMod::emitPolarPitchRmsPoints() {
  float pitch = getNormalisedAnalysisScalar(minPitchParameter, maxPitchParameter,
                                            ofxAudioAnalysisClient::AnalysisScalar::pitch);
  float rms = getNormalisedAnalysisScalar(minRmsParameter, maxRmsParameter,
                                          ofxAudioAnalysisClient::AnalysisScalar::rootMeanSquare);
  emit(SOURCE_POLAR_PITCH_RMS_POINTS, normalisedAngleLengthToPolar(pitch, rms));
}

void AudioDataSourceMod::emitSpectral2DPoints() {
//  float csd = getNormalisedAnalysisScalar(minComplexSpectralDifferenceParameter,
//                              maxComplexSpectralDifferenceParameter,
//                              ofxAudioAnalysisClient::AnalysisScalar::complexSpectralDifference);
  float sc = getNormalisedAnalysisScalar(minSpectralCrestParameter,
                              maxSpectralCrestParameter,
                              ofxAudioAnalysisClient::AnalysisScalar::spectralCrest);
  float zcr = getNormalisedAnalysisScalar(minZeroCrossingRateParameter,
                              maxZeroCrossingRateParameter,
                              ofxAudioAnalysisClient::AnalysisScalar::zeroCrossingRate);
//  emit(SOURCE_SPECTRAL_2D_POINTS, glm::vec2 { csd, sc });
  emit(SOURCE_SPECTRAL_2D_POINTS, glm::vec2 { sc, zcr });
}

void AudioDataSourceMod::emitPolarSpectral2DPoints() {
//  float csd = getNormalisedAnalysisScalar(minComplexSpectralDifferenceParameter,
//                              maxComplexSpectralDifferenceParameter,
//                              ofxAudioAnalysisClient::AnalysisScalar::complexSpectralDifference);
  float sc = getNormalisedAnalysisScalar(minSpectralCrestParameter,
                              maxSpectralCrestParameter,
                              ofxAudioAnalysisClient::AnalysisScalar::spectralCrest);
  float zcr = getNormalisedAnalysisScalar(minZeroCrossingRateParameter,
                              maxZeroCrossingRateParameter,
                              ofxAudioAnalysisClient::AnalysisScalar::zeroCrossingRate);
//  emit(SOURCE_POLAR_SPECTRAL_2D_POINTS, glm::vec2 { csd, sc });
  emit(SOURCE_POLAR_SPECTRAL_2D_POINTS, normalisedAngleLengthToPolar(sc, zcr));
}

void AudioDataSourceMod::emitSpectral3DPoints() {
  float x = getNormalisedAnalysisScalar(minComplexSpectralDifferenceParameter,
                              maxComplexSpectralDifferenceParameter,
                              ofxAudioAnalysisClient::AnalysisScalar::complexSpectralDifference);
  float y = getNormalisedAnalysisScalar(minSpectralCrestParameter,
                              maxSpectralCrestParameter,
                              ofxAudioAnalysisClient::AnalysisScalar::spectralCrest);
  float z = getNormalisedAnalysisScalar(minZeroCrossingRateParameter,
                              maxZeroCrossingRateParameter,
                              ofxAudioAnalysisClient::AnalysisScalar::zeroCrossingRate);
  emit(SOURCE_SPECTRAL_3D_POINTS, glm::vec3 { x, y, z });
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
      case SOURCE_SPECTRAL_3D_POINTS:
        emitSpectral3DPoints();
        break;
      case SOURCE_SPECTRAL_2D_POINTS:
        emitSpectral2DPoints();
        break;
      case SOURCE_POLAR_SPECTRAL_2D_POINTS:
        emitPolarSpectral2DPoints();
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
  float pitchChangeMagnitude = audioDataProcessorPtr->detectPitchChange1();
  if (pitchChangeMagnitude > 0.0f) emit(SOURCE_PITCH_CHANGE, pitchChangeMagnitude);
}

bool AudioDataSourceMod::keyPressed(int key) {
  if (audioAnalysisClientPtr->keyPressed(key)) return true;
  if (audioDataPlotsPtr->keyPressed(key)) return true;
  if (key == 'T') {
    tuningVisible = !tuningVisible;
    return true;
  }
  return false;
}

void AudioDataSourceMod::draw() {
  ofPushMatrix();
  audioDataPlotsPtr->drawPlots();
  ofPopMatrix();
  
  if (tuningVisible) {
    float pitch = getNormalisedAnalysisScalar(minPitchParameter, maxPitchParameter,
                                              ofxAudioAnalysisClient::AnalysisScalar::pitch);
    float rms = getNormalisedAnalysisScalar(minRmsParameter, maxRmsParameter,
                                            ofxAudioAnalysisClient::AnalysisScalar::rootMeanSquare);
    ofSetColor(ofColor::blue);
    ofFill();
    ofDrawRectangle(pitch, rms, 1.0f/100.0f, 1.0f/100.0f);
    
    //  float csd = getNormalisedAnalysisScalar(minComplexSpectralDifferenceParameter,
    //                              maxComplexSpectralDifferenceParameter,
    //                              ofxAudioAnalysisClient::AnalysisScalar::complexSpectralDifference);
    float sc = getNormalisedAnalysisScalar(minSpectralCrestParameter,
                                maxSpectralCrestParameter,
                                ofxAudioAnalysisClient::AnalysisScalar::spectralCrest);
    float zcr = getNormalisedAnalysisScalar(minZeroCrossingRateParameter,
                                maxZeroCrossingRateParameter,
                                ofxAudioAnalysisClient::AnalysisScalar::zeroCrossingRate);
    ofSetColor(ofColor::purple);
    ofFill();
//      ofDrawRectangle(csd, sc, 1.0f/100.0f, 1.0f/100.0f);
    ofDrawRectangle(sc, zcr, 1.0f/100.0f, 1.0f/100.0f);
  }
}

void AudioDataSourceMod::shutdown() {
  audioAnalysisClientPtr->stopRecording();
  audioAnalysisClientPtr->closeStream();
}


} // ofxMarkSynth
