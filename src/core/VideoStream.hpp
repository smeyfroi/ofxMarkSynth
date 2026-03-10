//
//  VideoStream.hpp
//  ofxMarkSynth
//
//  Persistent video capture/playback stream owned by Synth.
//  Provides GPU double-buffered frames via PingPongFbo.
//

#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

#include "PingPongFbo.h"
#include "ofFbo.h"
#include "ofVideoGrabber.h"
#include "ofVideoPlayer.h"

#include <glm/vec2.hpp>

namespace ofxMarkSynth {

class VideoStream {
public:
  enum class Mode {
    None,
    Camera,
    File,
  };

  VideoStream() = default;
  ~VideoStream();

  // Camera mode
  bool setupCamera(int deviceId, glm::vec2 desiredSize);
  bool setupCamera(const std::string& deviceName, glm::vec2 desiredSize);

  // File mode
  bool setupFile(const std::filesystem::path& path, bool mute, std::optional<int> startPositionSeconds);

  void stop();
  void update();

  Mode getMode() const { return mode_; }
  bool isAllocated() const { return frameFbo_.getSource().isAllocated(); }
  bool isReady() const { return isAllocated() && hasEverReceivedFrame_; }

  glm::vec2 getSize() const { return size_; }

  // GPU frames: source = current, target = previous.
  const ofFbo& getCurrentFrameFbo() const { return frameFbo_.getSource(); }
  const ofFbo& getPreviousFrameFbo() const { return frameFbo_.getTarget(); }

  bool isFrameNew() const { return frameNewThisUpdate_; }
  std::uint64_t getFrameIndex() const { return frameIndex_; }

private:
  void allocateFbos(glm::vec2 size);
  void drawTextureToCurrentFrame(const ofTexture& texture, bool mirrorX);

  Mode mode_ { Mode::None };

  ofVideoGrabber grabber_;
  ofVideoPlayer player_;

  glm::vec2 size_ { 0.0f, 0.0f };
  PingPongFbo frameFbo_;

  bool frameNewThisUpdate_ { false };
  bool hasEverReceivedFrame_ { false };
  std::uint64_t frameIndex_ { 0 };
};

} // namespace ofxMarkSynth
