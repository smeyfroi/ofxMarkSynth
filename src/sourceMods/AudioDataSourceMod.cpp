//
//  AudioDataSourceMod.cpp
//  example_audio
//
//  Created by Steve Meyfroidt on 03/05/2025.
//

#include "AudioDataSourceMod.hpp"


namespace ofxMarkSynth {


AudioDataSourceMod::AudioDataSourceMod(const std::string& name, const ModConfig&& config)
: Mod { name, std::move(config) }
{}

void AudioDataSourceMod::initParameters() {
  parameters.add(minPitchParameter);
  parameters.add(maxPitchParameter);
  parameters.add(minRmsParameter);
  parameters.add(maxRmsParameter);
}

void AudioDataSourceMod::update() {
  if (!audioDataProcessorPtr) { ofLogError() << "update in " << typeid(*this).name() << " with no audioDataProcessor"; return; }
  if (audioDataProcessorPtr->isDataValid()) {
    // TODO: make the dimensions configurable
    float x, y;

    if (minPitchParameter == 0.0 && maxPitchParameter == 0.0) {
      x = audioDataProcessorPtr->getNormalisedScalarValue(ofxAudioAnalysisClient::AnalysisScalar::pitch);
    } else {
      x = audioDataProcessorPtr->getNormalisedScalarValue(ofxAudioAnalysisClient::AnalysisScalar::pitch, minPitchParameter, maxPitchParameter);
    }

    if (minRmsParameter == 0.0 && maxRmsParameter == 0.0) {
      y = audioDataProcessorPtr->getNormalisedScalarValue(ofxAudioAnalysisClient::AnalysisScalar::rootMeanSquare);
    } else {
      y = audioDataProcessorPtr->getNormalisedScalarValue(ofxAudioAnalysisClient::AnalysisScalar::rootMeanSquare, minRmsParameter, maxRmsParameter);
    }

    std::for_each(connections[SOURCE_POINTS]->begin(),
                  connections[SOURCE_POINTS]->end(),
                  [&](auto& p) {
      auto& [modPtr, sinkId] = p;
      modPtr->receive(sinkId, glm::vec2 { x, y });
    });
  }
}


} // ofxMarkSynth
