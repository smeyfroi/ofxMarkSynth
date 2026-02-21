//  SessionResourceUtil.h
//  ofxMarkSynth
//
//  Parse a flat "session config" JSON into a ResourceManager.
//  Convention: underscore-prefixed keys are treated as comments/inactive.
//

#pragma once

#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>

#include "ofMain.h"
#include "ofxTimeMeasurements.h"

#include "config/ModFactory.hpp"
#include "core/FontStash2Cache.hpp"
#include "util/SessionConfigUtil.h"

namespace ofxMarkSynth {

inline bool isUnderscoreKey(const std::string& key) {
  return !key.empty() && key[0] == '_';
}

inline std::filesystem::path expandUserPath(const std::string& pathStr) {
  if (pathStr == "~") {
    return std::filesystem::path(ofFilePath::getUserHomeDir());
  }

  if (pathStr.rfind("~/", 0) == 0) {
    auto home = std::filesystem::path(ofFilePath::getUserHomeDir());
    return home / pathStr.substr(2);
  }

  return std::filesystem::path(pathStr);
}

inline std::optional<std::string> getStringValue(const ofJson& j, const std::string& key) {
  if (!j.contains(key) || !j[key].is_string()) {
    return std::nullopt;
  }
  return j[key].get<std::string>();
}

inline std::optional<bool> getBoolValue(const ofJson& j, const std::string& key) {
  if (!j.contains(key) || !j[key].is_boolean()) {
    return std::nullopt;
  }
  return j[key].get<bool>();
}

inline std::optional<int> getIntValue(const ofJson& j, const std::string& key) {
  if (!j.contains(key) || !j[key].is_number_integer()) {
    return std::nullopt;
  }
  return j[key].get<int>();
}

inline std::optional<float> getFloatValue(const ofJson& j, const std::string& key) {
  if (!j.contains(key) || !(j[key].is_number_float() || j[key].is_number_integer())) {
    return std::nullopt;
  }
  return j[key].get<float>();
}

inline std::optional<glm::vec2> getVec2Value(const ofJson& j, const std::string& key) {
  if (!j.contains(key) || !j[key].is_array() || j[key].size() != 2) {
    return std::nullopt;
  }

  const auto& a = j[key];
  if (!(a[0].is_number() && a[1].is_number())) {
    return std::nullopt;
  }

  return glm::vec2 { a[0].get<float>(), a[1].get<float>() };
}

inline std::optional<ofLogLevel> parseLogLevelString(const std::string& levelStr) {
  const std::string s = ofToLower(levelStr);
  if (s == "verbose") return OF_LOG_VERBOSE;
  if (s == "notice") return OF_LOG_NOTICE;
  if (s == "warning") return OF_LOG_WARNING;
  if (s == "error") return OF_LOG_ERROR;
  if (s == "fatal") return OF_LOG_FATAL_ERROR;
  if (s == "silent") return OF_LOG_SILENT;
  return std::nullopt;
}

inline void applySessionRuntimeSettings(const ofJson& sessionJson) {
  const float frameRate = getFloatValue(sessionJson, "frameRate").value_or(30.0f);
  ofSetFrameRate(frameRate);
  TIME_SAMPLE_SET_FRAMERATE(frameRate);

  const bool timeMeasurementsEnabled = getBoolValue(sessionJson, "timeMeasurementsEnabled").value_or(false);
  if (timeMeasurementsEnabled) {
    TIME_SAMPLE_ENABLE();
  } else {
    TIME_SAMPLE_DISABLE();
  }

  if (auto logLevelStrOpt = getStringValue(sessionJson, "logLevel"); logLevelStrOpt && !logLevelStrOpt->empty()) {
    if (auto levelOpt = parseLogLevelString(*logLevelStrOpt)) {
      ofSetLogLevel(*levelOpt);
    } else {
      ofLogWarning("SessionResourceUtil") << "Unknown logLevel: " << *logLevelStrOpt;
    }
  }

  if (auto destOpt = getStringValue(sessionJson, "logDestination"); destOpt) {
    const std::string dest = ofToLower(*destOpt);
    if (dest == "console") {
      ofLogToConsole();
    }
  }
}

inline ResourceManager buildResourceManagerFromSessionConfigJson(const ofJson& sessionJson) {
  if (!sessionJson.is_object()) {
    throw std::runtime_error("Session config must be a JSON object");
  }

  ResourceManager resources;

  // === ROOT PATHS ===
  const auto rootSourceMaterialPathStrOpt = getStringValue(sessionJson, "rootSourceMaterialPath");
  const auto rootPerformancePathStrOpt = getStringValue(sessionJson, "rootPerformancePath");
  if (!rootSourceMaterialPathStrOpt || rootSourceMaterialPathStrOpt->empty()) {
    throw std::runtime_error("Missing required string key: rootSourceMaterialPath");
  }
  if (!rootPerformancePathStrOpt || rootPerformancePathStrOpt->empty()) {
    throw std::runtime_error("Missing required string key: rootPerformancePath");
  }

  const auto rootSourceMaterialPath = expandUserPath(*rootSourceMaterialPathStrOpt);
  const auto rootPerformancePath = expandUserPath(*rootPerformancePathStrOpt);

  const std::filesystem::path performanceConfigRootPath = rootPerformancePath / "config";
  const std::filesystem::path performanceArtefactRootPath = rootPerformancePath / "artefact";

  resources.add("performanceConfigRootPath", performanceConfigRootPath);
  resources.add("performanceArtefactRootPath", performanceArtefactRootPath);

  // === REQUIRED SYNTH RESOURCES ===
  const auto compositeSizeOpt = getVec2Value(sessionJson, "compositeSize");
  const auto startHibernatedOpt = getBoolValue(sessionJson, "startHibernated");
  if (!compositeSizeOpt) {
    throw std::runtime_error("Missing required [w,h] key: compositeSize");
  }
  if (!startHibernatedOpt) {
    throw std::runtime_error("Missing required bool key: startHibernated");
  }
  resources.add("compositeSize", *compositeSizeOpt);
  resources.add("startHibernated", *startHibernatedOpt);

  // === SYNTH/RECORDING RESOURCES ===
  const auto compositePanelGapPxOpt = getFloatValue(sessionJson, "compositePanelGapPx");
  const auto recorderCompositeSizeOpt = getVec2Value(sessionJson, "recorderCompositeSize");
  const auto ffmpegBinaryPathStrOpt = getStringValue(sessionJson, "ffmpegBinaryPath");
  if (!compositePanelGapPxOpt) {
    throw std::runtime_error("Missing required number key: compositePanelGapPx");
  }
  if (!recorderCompositeSizeOpt) {
    throw std::runtime_error("Missing required [w,h] key: recorderCompositeSize");
  }
  if (!ffmpegBinaryPathStrOpt || ffmpegBinaryPathStrOpt->empty()) {
    throw std::runtime_error("Missing required string key: ffmpegBinaryPath");
  }

  resources.add("compositePanelGapPx", *compositePanelGapPxOpt);
  resources.add("recorderCompositeSize", *recorderCompositeSizeOpt);
  resources.add("ffmpegBinaryPath", expandUserPath(*ffmpegBinaryPathStrOpt));

  // Startup performance config name (may be empty)
  if (auto startupNameOpt = getStringValue(sessionJson, "startupPerformanceConfigName")) {
    resources.add("startupPerformanceConfigName", *startupNameOpt);
  }

  // Logging destination (optional, default console)
  std::string logDestination = "console";
  if (auto logDestinationOpt = getStringValue(sessionJson, "logDestination"); logDestinationOpt && !logDestinationOpt->empty()) {
    const std::string s = ofToLower(*logDestinationOpt);
    if (s == "console" || s == "gui") {
      logDestination = s;
    } else {
      ofLogWarning("SessionResourceUtil") << "Unknown logDestination: " << *logDestinationOpt;
    }
  }
  resources.add("logDestination", logDestination);

  // Recording + mux settings
  const bool startRecordingOnFirstWake = getBoolValue(sessionJson, "startRecordingOnFirstWake").value_or(false);
  resources.add("startRecordingOnFirstWake", startRecordingOnFirstWake);

  const int muxAudioBitrateKbps = getIntValue(sessionJson, "muxAudioBitrateKbps").value_or(192);
  resources.add("muxAudioBitrateKbps", muxAudioBitrateKbps);

  // Autosnapshots
  const bool autoSnapshotsEnabled = getBoolValue(sessionJson, "autoSnapshotsEnabled").value_or(false);
  const float autoSnapshotsIntervalSec = getFloatValue(sessionJson, "autoSnapshotsIntervalSec").value_or(20.0f);
  const float autoSnapshotsJitterSec = getFloatValue(sessionJson, "autoSnapshotsJitterSec").value_or(7.0f);
  resources.add("autoSnapshotsEnabled", autoSnapshotsEnabled);
  resources.add("autoSnapshotsIntervalSec", autoSnapshotsIntervalSec);
  resources.add("autoSnapshotsJitterSec", autoSnapshotsJitterSec);

  // === TEXT/FONT RESOURCES ===
  const auto fontFileOpt = getStringValue(sessionJson, "fontFile");
  const auto textSourcesDirOpt = getStringValue(sessionJson, "textSourcesDir");
  if (!fontFileOpt || fontFileOpt->empty()) {
    throw std::runtime_error("Missing required string key: fontFile");
  }
  if (!textSourcesDirOpt || textSourcesDirOpt->empty()) {
    throw std::runtime_error("Missing required string key: textSourcesDir");
  }

  const std::filesystem::path fontPath = performanceConfigRootPath / *fontFileOpt;
  auto fontCachePtr = std::make_shared<FontStash2Cache>(fontPath);
  fontCachePtr->setup();
  fontCachePtr->prewarmAll();
  resources.addShared("fontCache", fontCachePtr);

  const std::filesystem::path textSourcesPath = performanceConfigRootPath / *textSourcesDirOpt;
  resources.add("textSourcesPath", textSourcesPath.string());

  // === AUDIO INPUT (file OR mic) ===
  if (auto sourceAudioFileOpt = getStringValue(sessionJson, "sourceAudioFile"); sourceAudioFileOpt && !sourceAudioFileOpt->empty()) {
    ofLogNotice("SessionResourceUtil") << "Audio mode: file playback";
    resources.add("sourceAudioPath", rootSourceMaterialPath / *sourceAudioFileOpt);

    const auto audioOutDeviceNameOpt = getStringValue(sessionJson, "audioOutDeviceName");
    const auto audioBufferSizeOpt = getIntValue(sessionJson, "audioBufferSize");
    const auto audioChannelsOpt = getIntValue(sessionJson, "audioChannels");
    const auto audioSampleRateOpt = getIntValue(sessionJson, "audioSampleRate");
    if (!audioOutDeviceNameOpt || audioOutDeviceNameOpt->empty() || !audioBufferSizeOpt || !audioChannelsOpt || !audioSampleRateOpt) {
      throw std::runtime_error("Audio file mode requires audioOutDeviceName, audioBufferSize, audioChannels, audioSampleRate");
    }

    resources.add("audioOutDeviceName", *audioOutDeviceNameOpt);
    resources.add("audioBufferSize", *audioBufferSizeOpt);
    resources.add("audioChannels", *audioChannelsOpt);
    resources.add("audioSampleRate", *audioSampleRateOpt);

    if (auto startPosOpt = getStringValue(sessionJson, "sourceAudioStartPosition"); startPosOpt && !startPosOpt->empty()) {
      resources.add("sourceAudioStartPosition", *startPosOpt);
    }
  } else {
    ofLogNotice("SessionResourceUtil") << "Audio mode: microphone";
    const auto micDeviceNameOpt = getStringValue(sessionJson, "micDeviceName");
    const auto recordAudioOpt = getBoolValue(sessionJson, "recordAudio");
    const auto audioRecordingDirOpt = getStringValue(sessionJson, "audioRecordingDir");
    if (!micDeviceNameOpt || micDeviceNameOpt->empty() || !recordAudioOpt || !audioRecordingDirOpt || audioRecordingDirOpt->empty()) {
      throw std::runtime_error("Mic mode requires micDeviceName, recordAudio, audioRecordingDir");
    }

    resources.add("micDeviceName", *micDeviceNameOpt);
    resources.add("recordAudio", *recordAudioOpt);
    resources.add("audioRecordingPath", performanceArtefactRootPath / *audioRecordingDirOpt);
  }

  // === VIDEO INPUT (file OR camera) ===
  if (auto sourceVideoFileOpt = getStringValue(sessionJson, "sourceVideoFile"); sourceVideoFileOpt && !sourceVideoFileOpt->empty()) {
    ofLogNotice("SessionResourceUtil") << "Video mode: file playback";
    resources.add("sourceVideoPath", rootSourceMaterialPath / *sourceVideoFileOpt);

    const auto sourceVideoMuteOpt = getBoolValue(sessionJson, "sourceVideoMute");
    if (!sourceVideoMuteOpt) {
      throw std::runtime_error("Video file mode requires sourceVideoMute");
    }
    resources.add("sourceVideoMute", *sourceVideoMuteOpt);

    if (auto startPosOpt = getStringValue(sessionJson, "sourceVideoStartPosition"); startPosOpt && !startPosOpt->empty()) {
      resources.add("sourceVideoStartPosition", *startPosOpt);
    }
  } else {
    ofLogNotice("SessionResourceUtil") << "Video mode: camera";
    const auto cameraDeviceIdOpt = getIntValue(sessionJson, "cameraDeviceId");
    const auto videoSizeOpt = getVec2Value(sessionJson, "videoSize");
    const auto saveRecordingOpt = getBoolValue(sessionJson, "saveRecording");
    const auto videoRecordingDirOpt = getStringValue(sessionJson, "videoRecordingDir");
    if (!cameraDeviceIdOpt || !videoSizeOpt || !saveRecordingOpt || !videoRecordingDirOpt || videoRecordingDirOpt->empty()) {
      throw std::runtime_error("Video camera mode requires cameraDeviceId, videoSize, saveRecording, videoRecordingDir");
    }

    resources.add("cameraDeviceId", *cameraDeviceIdOpt);
    resources.add("videoSize", *videoSizeOpt);
    resources.add("saveRecording", *saveRecordingOpt);
    resources.add("videoRecordingPath", performanceArtefactRootPath / *videoRecordingDirOpt);
  }

  return resources;
}

inline ResourceManager loadResourceManagerFromSessionConfigJsonOrExit(const ofJson& sessionJson, const std::string& appNamespace) {
  try {
    return buildResourceManagerFromSessionConfigJson(sessionJson);
  } catch (const std::exception& e) {
    hardExitWithAlert(std::string("Failed to parse session config for '") + appNamespace + "': " + e.what());
  }
}

inline ResourceManager loadSessionResourceManagerOrExit(const SessionConfigSelectorOptions& options) {
  const ofJson sessionJson = loadSessionConfigJsonOrExit(options);
  applySessionRuntimeSettings(sessionJson);
  return loadResourceManagerFromSessionConfigJsonOrExit(sessionJson, options.appNamespace);
}

} // namespace ofxMarkSynth
