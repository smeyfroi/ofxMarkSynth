//
//  AudioDataSourceMod.cpp
//  example_audio
//
//  Created by Steve Meyfroidt on 03/05/2025.
//

#include "AudioDataSourceMod.hpp"
#include <algorithm>
#include <cmath>
#include <limits>



namespace ofxMarkSynth {



AudioDataSourceMod::AudioDataSourceMod(std::shared_ptr<Synth> synthPtr,
                                       const std::string& name,
                                       ModConfig config,
                                       std::shared_ptr<ofxAudioAnalysisClient::LocalGistClient> audioAnalysisClient)
: Mod { synthPtr, name, std::move(config) }
, audioAnalysisClientPtr { std::move(audioAnalysisClient) }
{
  initialise();
}

void AudioDataSourceMod::initialise() {
  audioDataProcessorPtr = std::make_shared<ofxAudioData::Processor>(audioAnalysisClientPtr);
  audioDataProcessorPtr->setDefaultValiditySpecs();
  audioDataPlotsPtr = std::make_shared<ofxAudioData::Plots>(audioDataProcessorPtr);
  
  sourceNameIdMap = {
    { "PitchRmsPoint", SOURCE_PITCH_RMS_POINTS },
    { "PolarPitchRmsPoint", SOURCE_POLAR_PITCH_RMS_POINTS },
    { "DriftPitchRmsPoint", SOURCE_DRIFT_PITCH_RMS_POINTS },
    { "Spectral3dPoint", SOURCE_SPECTRAL_3D_POINTS },
    { "Spectral2dPoint", SOURCE_SPECTRAL_2D_POINTS },
    { "DriftSpectral2dPoint", SOURCE_DRIFT_SPECTRAL_2D_POINTS },
    { "PolarSpectral2dPoint", SOURCE_POLAR_SPECTRAL_2D_POINTS },
    { "PitchScalar", SOURCE_PITCH_SCALAR },
    { "RmsScalar", SOURCE_RMS_SCALAR },

    { "SpectralCentroidScalar", SOURCE_SPECTRAL_CENTROID_SCALAR },

    { "SpectralCrestScalar", SOURCE_SPECTRAL_CREST_SCALAR },
    { "ZeroCrossingRateScalar", SOURCE_ZERO_CROSSING_RATE_SCALAR },
    { "Onset1", SOURCE_ONSET1 },
    { "TimbreChange", SOURCE_TIMBRE_CHANGE },
    { "PitchChange", SOURCE_PITCH_CHANGE }
  };
}


void AudioDataSourceMod::initParameters() {
  parameters.add(scalarFilterIndexParameter);

  parameters.add(minPitchParameter);
  parameters.add(maxPitchParameter);
  parameters.add(minRmsParameter);
  parameters.add(maxRmsParameter);

  parameters.add(minSpectralCentroidParameter);
  parameters.add(maxSpectralCentroidParameter);

  parameters.add(minSpectralCrestParameter);
  parameters.add(maxSpectralCrestParameter);
  parameters.add(minZeroCrossingRateParameter);
  parameters.add(maxZeroCrossingRateParameter);

  parameters.add(driftFollowMinParameter);
  parameters.add(driftFollowMaxParameter);
  parameters.add(driftFollowGammaParameter);
  parameters.add(driftAccelParameter);
  parameters.add(driftDampingParameter);
  parameters.add(driftCenterSpringParameter);
  parameters.add(driftMaxVelocityParameter);

  // Keep this sub-group just for tuning (thresholds/cooldowns).
  parameters.add(audioDataProcessorPtr->getParameterGroup());
}

float AudioDataSourceMod::getNormalisedAnalysisScalar(float minParameter, float maxParameter, ofxAudioAnalysisClient::AnalysisScalar scalar) {
  return audioDataProcessorPtr->getNormalisedScalarValue(
      scalar,
      minParameter,
      maxParameter,
      scalarFilterIndexParameter.get(),
      true);
}

void AudioDataSourceMod::emitPitchRmsPoints() {
  float x = getNormalisedAnalysisScalar(minPitchParameter, maxPitchParameter,
                                        ofxAudioAnalysisClient::AnalysisScalar::pitch);
  float y = getNormalisedAnalysisScalar(minRmsParameter, maxRmsParameter,
                                        ofxAudioAnalysisClient::AnalysisScalar::rootMeanSquare);
  emit(SOURCE_PITCH_RMS_POINTS, glm::vec2 { x, y });
}

float AudioDataSourceMod::getRandomSignedFloat() {
  // xorshift32
  driftRngState ^= driftRngState << 13;
  driftRngState ^= driftRngState >> 17;
  driftRngState ^= driftRngState << 5;

  constexpr float UINT32_MAX_F = static_cast<float>(std::numeric_limits<uint32_t>::max());
  float unit = static_cast<float>(driftRngState) / UINT32_MAX_F;
  return unit * 2.0f - 1.0f;
}

float AudioDataSourceMod::updateDriftY(float yRaw, float& yState, float& yVelocity) {
  float y = std::clamp(yRaw, 0.0f, 1.0f);

  float followMin = std::clamp(driftFollowMinParameter.get(), 0.0f, 1.0f);
  float followMax = std::clamp(driftFollowMaxParameter.get(), 0.0f, 1.0f);

  float t = 0.0f;
  if (followMax > followMin + 0.0001f) {
    t = (y - followMin) / (followMax - followMin);
  } else {
    t = (y >= followMin) ? 1.0f : 0.0f;
  }
  t = std::clamp(t, 0.0f, 1.0f);

  // smoothstep
  float follow = t * t * (3.0f - 2.0f * t);
  follow = std::pow(follow, driftFollowGammaParameter.get());
  follow = std::clamp(follow, 0.0f, 1.0f);

  // Loud passages: track the raw y mapping.
  yState = yState + (y - yState) * follow;

  // Quiet passages: mostly hold + drift.
  float quietFactor = 1.0f - follow;
  yVelocity += getRandomSignedFloat() * driftAccelParameter.get() * quietFactor;
  yVelocity -= driftCenterSpringParameter.get() * quietFactor * (yState - 0.5f);
  yVelocity *= driftDampingParameter.get();

  float maxVel = driftMaxVelocityParameter.get();
  yVelocity = std::clamp(yVelocity, -maxVel, maxVel);

  yState += yVelocity;

  // Wrap to [0, 1].
  yState = std::fmod(yState, 1.0f);
  if (yState < 0.0f) yState += 1.0f;

  return yState;
}

void AudioDataSourceMod::emitDriftPitchRmsPoints() {
  float x = getNormalisedAnalysisScalar(minPitchParameter, maxPitchParameter,
                                        ofxAudioAnalysisClient::AnalysisScalar::pitch);
  float yRaw = getNormalisedAnalysisScalar(minRmsParameter, maxRmsParameter,
                                           ofxAudioAnalysisClient::AnalysisScalar::rootMeanSquare);
  float y = updateDriftY(yRaw, driftPitchRmsYState, driftPitchRmsYVelocity);
  emit(SOURCE_DRIFT_PITCH_RMS_POINTS, glm::vec2 { x, y });
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
  float centroid = getNormalisedAnalysisScalar(minSpectralCentroidParameter,
                                               maxSpectralCentroidParameter,
                                               ofxAudioAnalysisClient::AnalysisScalar::spectralCentroid);
  float crest = getNormalisedAnalysisScalar(minSpectralCrestParameter,
                                            maxSpectralCrestParameter,
                                            ofxAudioAnalysisClient::AnalysisScalar::spectralCrest);
  emit(SOURCE_SPECTRAL_2D_POINTS, glm::vec2 { centroid, crest });
}

void AudioDataSourceMod::emitDriftSpectral2DPoints() {
  float centroid = getNormalisedAnalysisScalar(minSpectralCentroidParameter,
                                               maxSpectralCentroidParameter,
                                               ofxAudioAnalysisClient::AnalysisScalar::spectralCentroid);
  float crestRaw = getNormalisedAnalysisScalar(minSpectralCrestParameter,
                                               maxSpectralCrestParameter,
                                               ofxAudioAnalysisClient::AnalysisScalar::spectralCrest);
  float crest = updateDriftY(crestRaw, driftSpectral2DYState, driftSpectral2DYVelocity);
  emit(SOURCE_DRIFT_SPECTRAL_2D_POINTS, glm::vec2 { centroid, crest });
}

void AudioDataSourceMod::emitPolarSpectral2DPoints() {
  float centroid = getNormalisedAnalysisScalar(minSpectralCentroidParameter,
                                               maxSpectralCentroidParameter,
                                               ofxAudioAnalysisClient::AnalysisScalar::spectralCentroid);
  float crest = getNormalisedAnalysisScalar(minSpectralCrestParameter,
                                            maxSpectralCrestParameter,
                                            ofxAudioAnalysisClient::AnalysisScalar::spectralCrest);
  emit(SOURCE_POLAR_SPECTRAL_2D_POINTS, normalisedAngleLengthToPolar(centroid, crest));
}


void AudioDataSourceMod::emitSpectral3DPoints() {
  float x = getNormalisedAnalysisScalar(minSpectralCentroidParameter,
                                        maxSpectralCentroidParameter,
                                        ofxAudioAnalysisClient::AnalysisScalar::spectralCentroid);
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
  syncControllerAgencies();
  if (!audioDataProcessorPtr) { ofLogError("AudioDataSourceMod") << "Update called with no audioDataProcessor"; return; }
  
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
      case SOURCE_DRIFT_PITCH_RMS_POINTS:
        emitDriftPitchRmsPoints();
        break;
      case SOURCE_SPECTRAL_3D_POINTS:
        emitSpectral3DPoints();
        break;
      case SOURCE_SPECTRAL_2D_POINTS:
        emitSpectral2DPoints();
        break;
      case SOURCE_DRIFT_SPECTRAL_2D_POINTS:
        emitDriftSpectral2DPoints();
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
      case SOURCE_SPECTRAL_CENTROID_SCALAR:
        emitScalar(SOURCE_SPECTRAL_CENTROID_SCALAR,
                   minSpectralCentroidParameter,
                   maxSpectralCentroidParameter,
                   ofxAudioAnalysisClient::AnalysisScalar::spectralCentroid);
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
  if (key == 't') {
    tuningVisible = !tuningVisible;
    return true;
  }
  return false;
}

void AudioDataSourceMod::drawEventDetectionOverlay() {
  // This draws in a [0,1] normalized coordinate system (parent has already scaled)
  // We need to draw text at a fixed pixel size, so we scale down to pixel coords
  
  // Get the current modelview matrix to determine the effective scale
  // The parent has scaled by the viewport size, so we can use that
  ofRectangle viewport = ofGetCurrentViewport();
  float effectiveScale = std::max(viewport.width, viewport.height);
  
  // Text character size in normalized coords (8 pixels / effectiveScale)
  float charW = 8.0f / effectiveScale;
  float charH = 13.0f / effectiveScale;  // bitmap font is ~13px tall
  
  // Compact panel in bottom-left - sizes relative to normalized [0,1] space
  constexpr float PANEL_MARGIN = 0.01f;
  float barHeight = charH * 0.8f;
  float cooldownHeight = charH * 0.2f;
  float rowGap = charH * 0.3f;
  float labelWidth = charW * 7.0f;  // "Timbre" is 6 chars + padding
  float barWidth = 0.12f;
  constexpr float MAX_ZSCORE = 5.0f;
  float padding = charW * 0.5f;

  // Calculate panel dimensions based on content
  float panelWidth = labelWidth + barWidth + padding * 3;
  float contentHeight = 3 * (barHeight + cooldownHeight + rowGap) + 2 * charH; // 3 bars + 2 legend lines
  float panelHeight = contentHeight + padding * 2;

  // Position in bottom-left
  float panelX = PANEL_MARGIN;
  float panelY = 1.0f - panelHeight - PANEL_MARGIN;

  ofPushStyle();

  // Semi-transparent background
  ofSetColor(0, 0, 0, 100);
  ofFill();
  ofDrawRectangle(panelX, panelY, panelWidth, panelHeight);

  // Helper lambda to draw one detector row
  auto drawDetectorRow = [&](float y, const std::string& label, float zScore, float threshold,
                             float cooldownRemaining, float cooldownTotal, float flash) {
    float barX = panelX + labelWidth + padding;

    // Label - draw text using normalized scale
    ofSetColor(255);
    ofPushMatrix();
    ofTranslate(panelX + padding, y + barHeight * 0.8f);
    ofScale(1.0f / effectiveScale, 1.0f / effectiveScale);
    ofDrawBitmapString(label, 0, 0);
    ofPopMatrix();

    // Z-score bar background
    ofSetColor(40, 40, 40);
    ofDrawRectangle(barX, y, barWidth, barHeight);

    // Calculate fill ratio and color based on proximity to threshold
    float ratio = std::min(zScore / MAX_ZSCORE, 1.0f);
    float thresholdRatio = threshold / MAX_ZSCORE;
    float proximityToThreshold = (threshold > 0.001f) ? zScore / threshold : 0.0f;

    ofColor barColor;
    if (proximityToThreshold < 0.5f) {
      barColor = ofColor(0, 180, 0); // Green
    } else if (proximityToThreshold < 0.9f) {
      barColor = ofColor(200, 200, 0); // Yellow
    } else if (proximityToThreshold < 1.0f) {
      barColor = ofColor(255, 140, 0); // Orange
    } else {
      barColor = ofColor(255, 50, 50); // Red
    }

    // Draw z-score fill
    ofSetColor(barColor);
    ofDrawRectangle(barX, y, barWidth * ratio, barHeight);

    // Threshold marker (vertical line)
    ofSetColor(255, 255, 255, 200);
    float thresholdX = barX + barWidth * thresholdRatio;
    ofDrawRectangle(thresholdX, y, 0.002f, barHeight);

    // Flash overlay when triggered
    if (flash > 0.0f) {
      ofSetColor(255, 255, 255, static_cast<int>(flash * 200));
      ofDrawRectangle(barX, y, barWidth, barHeight);
    }

    // Cooldown progress bar (thin line below)
    if (cooldownRemaining > 0.0f && cooldownTotal > 0.0f) {
      float cooldownRatio = cooldownRemaining / cooldownTotal;
      ofSetColor(100, 100, 100);
      ofDrawRectangle(barX, y + barHeight + 0.001f, barWidth * cooldownRatio, cooldownHeight);
    }
  };

  // Draw the three detector rows
  float rowY = panelY + padding;

  drawDetectorRow(rowY, "Onset",
                  audioDataProcessorPtr->getOnsetZScore(),
                  audioDataProcessorPtr->getOnsetThreshold(),
                  audioDataProcessorPtr->getOnsetCooldownRemaining(),
                  audioDataProcessorPtr->getOnsetCooldownTotal(),
                  audioDataProcessorPtr->getOnsetTriggerFlash());

  rowY += barHeight + cooldownHeight + rowGap;

  drawDetectorRow(rowY, "Timbre",
                  audioDataProcessorPtr->getTimbreZScore(),
                  audioDataProcessorPtr->getTimbreThreshold(),
                  audioDataProcessorPtr->getTimbreCooldownRemaining(),
                  audioDataProcessorPtr->getTimbreCooldownTotal(),
                  audioDataProcessorPtr->getTimbreTriggerFlash());

  rowY += barHeight + cooldownHeight + rowGap;

  drawDetectorRow(rowY, "Pitch",
                  audioDataProcessorPtr->getPitchZScore(),
                  audioDataProcessorPtr->getPitchThreshold(),
                  audioDataProcessorPtr->getPitchCooldownRemaining(),
                  audioDataProcessorPtr->getPitchCooldownTotal(),
                  audioDataProcessorPtr->getPitchTriggerFlash());

  // Legend for existing blue/purple dots
  rowY += barHeight + cooldownHeight + rowGap;

  ofSetColor(100, 100, 255); // Blue
  ofPushMatrix();
  ofTranslate(panelX + padding, rowY + charH);
  ofScale(1.0f / effectiveScale, 1.0f / effectiveScale);
  ofDrawBitmapString("Blue: Pitch RMS", 0, 0);
  ofPopMatrix();

  rowY += charH;
  ofSetColor(180, 100, 180); // Purple
  ofPushMatrix();
  ofTranslate(panelX + padding, rowY + charH);
  ofScale(1.0f / effectiveScale, 1.0f / effectiveScale);
  ofDrawBitmapString("Purple: Crest ZCR", 0, 0);
  ofPopMatrix();

  ofPopStyle();
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
    
    float sc = getNormalisedAnalysisScalar(minSpectralCrestParameter,
                                maxSpectralCrestParameter,
                                ofxAudioAnalysisClient::AnalysisScalar::spectralCrest);
    float zcr = getNormalisedAnalysisScalar(minZeroCrossingRateParameter,
                                maxZeroCrossingRateParameter,
                                ofxAudioAnalysisClient::AnalysisScalar::zeroCrossingRate);
    ofSetColor(ofColor::purple);
    ofFill();
    ofDrawRectangle(sc, zcr, 1.0f/100.0f, 1.0f/100.0f);

    drawEventDetectionOverlay();
  }
}

void AudioDataSourceMod::shutdown() {
}



} // ofxMarkSynth
