//
//  VideoFlowSourceMod.cpp
//  example_particles_from_video
//
//  Created by Steve Meyfroidt on 09/05/2025.
//

#include "VideoFlowSourceMod.hpp"
#include "core/IntentMapping.hpp"
#include "config/Parameter.hpp"
#include "core/IntentMapper.hpp"
#include "util/TimeStringUtil.h"

#include <algorithm>
#include <cmath>



namespace ofxMarkSynth {



VideoFlowSourceMod::VideoFlowSourceMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config, const std::filesystem::path& sourceVideoFilePath, bool mute, const std::string& startPosition)
: Mod { synthPtr, name, std::move(config) },
saveRecording { false }
{
  motionFromVideo.load(sourceVideoFilePath, mute);
  
  if (!startPosition.empty()) {
    int seconds = parseTimeStringToSeconds(startPosition);
    if (seconds > 0) {
      motionFromVideo.setPositionSeconds(seconds);
    }
  }
  
  sourceNameIdMap = {
    { "FlowField", SOURCE_FLOW_FIELD },
    { "PointVelocity", SOURCE_POINT_VELOCITY },
    { "Point", SOURCE_POINT }
  };

  registerControllerForSource(pointSamplesPerUpdateParameter, pointSamplesPerUpdateController);
}

VideoFlowSourceMod::VideoFlowSourceMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config, int deviceID, glm::vec2 size, bool saveRecording_, const std::filesystem::path& recordingDir_)
: Mod { synthPtr, name, std::move(config) },
saveRecording { saveRecording_ },
recordingDir { recordingDir_ }
{
  motionFromVideo.initialiseCamera(deviceID, size);
  
  sourceNameIdMap = {
    { "FlowField", SOURCE_FLOW_FIELD },
    { "PointVelocity", SOURCE_POINT_VELOCITY }
  };

  registerControllerForSource(pointSamplesPerUpdateParameter, pointSamplesPerUpdateController);
}

VideoFlowSourceMod::~VideoFlowSourceMod() {
#ifdef TARGET_MAC
  if (saveRecording) recorder.stop();
#endif
}

void VideoFlowSourceMod::shutdown() {
  motionFromVideo.stop();
}

#ifdef TARGET_MAC
void VideoFlowSourceMod::initRecorder() {
  recorder.setup(/*video*/true, /*audio*/false, motionFromVideo.getSize(), /*fps*/30.0, /*bitrate*/8000);
  recorder.setOverWrite(true);
  if (recordingDir == "") recordingDir = ofToDataPath("video-flow-recordings");
  std::filesystem::create_directory(recordingDir);
//  recorder.setFFmpegPathToAddonsPath();
  recorder.setFFmpegPath("/opt/homebrew/bin/ffmpeg");
//  recorder.setInputPixelFormat(ofPixelFormat::OF_PIXELS_RGB);
  recorder.setOutputPath(recordingDir+"/video-flow-recording-"+ofGetTimestampString()+".mp4");
  recorder.startCustomRecord();
}
#endif

namespace {

std::optional<std::reference_wrapper<ofAbstractParameter>> findParameterByNamePrefix(ofParameterGroup& group,
                                                                                   const std::string& namePrefix) {
  for (const auto& paramPtr : group) {
    if (paramPtr->getName().rfind(namePrefix, 0) == 0) {
      return std::ref(*paramPtr);
    }
    if (paramPtr->type() == typeid(ofParameterGroup).name()) {
      if (auto found = findParameterByNamePrefix(paramPtr->castGroup(), namePrefix)) {
        return found;
      }
    }
  }
  return std::nullopt;
}

void trySetParameterFromString(ofParameterGroup& group, const std::string& namePrefix, const std::string& v) {
  if (auto found = findParameterByNamePrefix(group, namePrefix)) {
    found->get().fromString(v);
  }
}

} // namespace

void VideoFlowSourceMod::initParameters() {
  parameters.add(pointSamplesPerUpdateParameter);
  parameters.add(pointSampleAttemptMultiplierParameter);
  parameters.add(agencyFactorParameter);

  // Default baseline (still overridden by presets/config/overrides when present).
  auto& motionParams = motionFromVideo.getParameterGroup();
  trySetParameterFromString(motionParams, "MinSpeedMagnitude", "0.40");

  addFlattenedParameterGroup(parameters, motionParams);
}

void VideoFlowSourceMod::update() {
  syncControllerAgencies();
  pointSamplesPerUpdateController.update();

  const bool hasPointSinks = connections.contains(SOURCE_POINT_VELOCITY) || connections.contains(SOURCE_POINT);
  const int pointSamplesPerUpdate = static_cast<int>(pointSamplesPerUpdateController.value);
  const float attemptMultiplier = std::max(1.0f, pointSampleAttemptMultiplierParameter.get());
  const int sampleAttemptsPerUpdate = std::min(1000, static_cast<int>(std::round(pointSamplesPerUpdate * attemptMultiplier)));
  motionFromVideo.setCpuSamplingEnabled(hasPointSinks && sampleAttemptsPerUpdate > 0);

  motionFromVideo.update();

  if (motionFromVideo.isReady()) {
    emit(SOURCE_FLOW_FIELD, motionFromVideo.getMotionFbo().getTexture());
  }

  // TODO: make this a separate process Mod that can sample a texture
  motionSampleStats = {};
  motionSampleStats.cpuSamplingEnabled = motionFromVideo.isCpuSamplingEnabled();

  if (motionFromVideo.isReady() && motionFromVideo.isCpuSamplingEnabled()) {
    motionSampleStats.samplesAttempted = sampleAttemptsPerUpdate;

    int acceptedCount = 0;
    float speedSum = 0.0f;
    float speedMax = 0.0f;

    for (int i = 0; i < sampleAttemptsPerUpdate; i++) {
      if (auto vec = motionFromVideo.trySampleMotion()) {
        const auto v = vec.value();
        emit(SOURCE_POINT_VELOCITY, v);
        emit(SOURCE_POINT, glm::vec2 { v });

        // Convert back to flow-texture speed units (so it matches MinSpeedMagnitude).
        const float speed = std::sqrt(v.z * v.z + v.w * v.w) * motionFromVideo.getSize().x;
        speedSum += speed;
        speedMax = std::max(speedMax, speed);
        acceptedCount++;
      }
    }

    motionSampleStats.samplesAccepted = acceptedCount;
    motionSampleStats.acceptRate = (sampleAttemptsPerUpdate > 0) ? (static_cast<float>(acceptedCount) / sampleAttemptsPerUpdate) : 0.0f;
    motionSampleStats.acceptedSpeedMean = (acceptedCount > 0) ? (speedSum / static_cast<float>(acceptedCount)) : 0.0f;
    motionSampleStats.acceptedSpeedMax = speedMax;
  }
}

float VideoFlowSourceMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

void VideoFlowSourceMod::applyIntent(const Intent& intent, float strength) {
  IntentMap im(intent);

  // Density can increase sampling (more activity), but keep it near the tuned baseline.
  im.D().expAround(pointSamplesPerUpdateController,
                   strength,
                   2.0f,
                   Mapping::WithFractions{0.15f, 0.15f});
}

void VideoFlowSourceMod::draw() {
#ifdef TARGET_MAC
  if (saveRecording) {
    if (!recorder.isRecording()) initRecorder();
    ofPixels pixels;
    motionFromVideo.getVideoFbo().readToPixels(pixels);
    recorder.addFrame(pixels);
  }
#endif
}

bool VideoFlowSourceMod::keyPressed(int key) {
  return motionFromVideo.keyPressed(key);
}



} // ofxMarkSynth
