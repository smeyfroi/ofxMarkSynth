//  RecordingMuxUtil.h
//  ofxMarkSynth
//
//  Spawn a detached ffmpeg mux process (audio+video -> mp4).
//  Designed to run independently of the app so mux continues after exit.
//

#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

#include "ofMain.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace ofxMarkSynth {

inline std::filesystem::path getMuxedVideoPath(const std::filesystem::path& videoPath) {
  auto out = videoPath;
  out.replace_filename(videoPath.stem().string() + "-muxed" + videoPath.extension().string());
  return out;
}

inline std::filesystem::path getMuxLogPath(const std::filesystem::path& videoPath) {
  auto out = videoPath;
  out.replace_filename(videoPath.stem().string() + "-mux.log");
  return out;
}

inline bool spawnDetachedMuxProcess(const std::filesystem::path& ffmpegPath,
                                   const std::filesystem::path& videoPath,
                                   const std::filesystem::path& audioPath,
                                   int audioBitrateKbps) {
  if (ffmpegPath.empty()) {
    ofLogWarning("RecordingMuxUtil") << "spawnDetachedMuxProcess: empty ffmpegPath";
    return false;
  }

  if (videoPath.empty() || audioPath.empty()) {
    ofLogWarning("RecordingMuxUtil") << "spawnDetachedMuxProcess: empty video/audio path";
    return false;
  }

  const auto outputPath = getMuxedVideoPath(videoPath);
  const auto logPath = getMuxLogPath(videoPath);

  if (std::filesystem::exists(outputPath)) {
    ofLogNotice("RecordingMuxUtil") << "Mux output already exists, skipping: " << outputPath;
    return true;
  }

  pid_t pid = fork();
  if (pid < 0) {
    ofLogError("RecordingMuxUtil") << "fork() failed";
    return false;
  }

  if (pid > 0) {
    // Parent: reap first child immediately to avoid zombies.
    int status = 0;
    (void)waitpid(pid, &status, 0);
    ofLogNotice("RecordingMuxUtil") << "Spawned mux job for: " << videoPath;
    return true;
  }

  // First child: detach from controlling terminal, then double-fork.
  (void)setsid();

  pid_t pid2 = fork();
  if (pid2 < 0) {
    _exit(1);
  }

  if (pid2 > 0) {
    _exit(0);
  }

  // Grandchild: do the actual work.
  {
    int fd = open(logPath.string().c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd >= 0) {
      (void)dup2(fd, STDOUT_FILENO);
      (void)dup2(fd, STDERR_FILENO);
      close(fd);
    }
  }

  // Wait for audio file to become stable (segment recording stop is async).
  const auto stableDuration = std::chrono::milliseconds(1500);
  const auto timeoutDuration = std::chrono::seconds(30);
  const auto pollPeriod = std::chrono::milliseconds(250);

  const auto startTime = std::chrono::steady_clock::now();
  auto lastChangeTime = startTime;
  std::uintmax_t lastSize = 0;
  bool hasSeenFile = false;

  while (std::chrono::steady_clock::now() - startTime < timeoutDuration) {
    if (std::filesystem::exists(audioPath)) {
      hasSeenFile = true;
      std::error_code ec;
      std::uintmax_t size = std::filesystem::file_size(audioPath, ec);
      if (!ec) {
        if (size != lastSize) {
          lastSize = size;
          lastChangeTime = std::chrono::steady_clock::now();
        }

        if (size > 0 && std::chrono::steady_clock::now() - lastChangeTime >= stableDuration) {
          break;
        }
      }
    }

    std::this_thread::sleep_for(pollPeriod);
  }

  if (!hasSeenFile) {
    dprintf(STDERR_FILENO, "Warning: audio file never appeared: %s\n", audioPath.string().c_str());
  }

  if (audioBitrateKbps <= 0) {
    audioBitrateKbps = 192;
  }

  const std::string bitrateArg = std::to_string(audioBitrateKbps) + "k";

  std::vector<std::string> args {
      ffmpegPath.string(),
      "-y",
      "-i", videoPath.string(),
      "-i", audioPath.string(),
      "-c:v", "copy",
      "-c:a", "aac",
      "-b:a", bitrateArg,
      "-shortest",
      outputPath.string(),
  };

  std::vector<char*> argv;
  argv.reserve(args.size() + 1);
  for (auto& s : args) {
    argv.push_back(s.data());
  }
  argv.push_back(nullptr);

  execv(ffmpegPath.string().c_str(), argv.data());

  dprintf(STDERR_FILENO, "Error: execv failed for ffmpeg: %s\n", ffmpegPath.string().c_str());
  _exit(1);
}

} // namespace ofxMarkSynth
