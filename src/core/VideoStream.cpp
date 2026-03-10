//
//  VideoStream.cpp
//  ofxMarkSynth
//

#include "core/VideoStream.hpp"

#include <algorithm>

#include "ofGraphics.h"
#include "ofLog.h"
#include "ofPixels.h"
#include "util/SessionConfigUtil.h"

namespace ofxMarkSynth {

namespace {

constexpr int DEFAULT_VIDEO_FPS = 30;

std::optional<int> findUniqueCameraDeviceIdByName(ofVideoGrabber& grabber, const std::string& deviceName) {
  const auto devices = grabber.listDevices();

  std::optional<int> id;
  int matchCount = 0;

  for (const auto& d : devices) {
    if (d.deviceName == deviceName) {
      id = d.id;
      matchCount++;
    }
  }

  if (matchCount == 1 && id) {
    return id;
  }

  std::string message;
  if (matchCount == 0) {
    message = "No camera matches cameraDeviceName='" + deviceName + "'";
  } else {
    message = "Ambiguous cameraDeviceName='" + deviceName + "' (" + std::to_string(matchCount) + " matches)";
  }

  ofLogNotice("VideoStream") << "Available cameras:";
  for (const auto& d : devices) {
    ofLogNotice("VideoStream") << "  id=" << d.id << " available=" << (d.bAvailable ? "true" : "false")
                               << " name='" << d.deviceName << "'";
  }

  // Hard-exit after logging the available camera list.
  hardExitWithAlert(message);
}

} // namespace

VideoStream::~VideoStream() {
  stop();
}

bool VideoStream::setupCamera(int deviceId, glm::vec2 desiredSize) {
  stop();

  mode_ = Mode::Camera;

  grabber_.setDeviceID(deviceId);
  grabber_.setPixelFormat(OF_PIXELS_RGB);
  grabber_.setDesiredFrameRate(DEFAULT_VIDEO_FPS);

  const bool ok = grabber_.setup(static_cast<int>(desiredSize.x), static_cast<int>(desiredSize.y));
  if (!ok) {
    ofLogError("VideoStream") << "Failed to setup camera deviceId=" << deviceId
                              << " size=" << desiredSize.x << "x" << desiredSize.y;
    mode_ = Mode::None;
    return false;
  }

  allocateFbos(grabber_.getSize());

  ofLogNotice("VideoStream") << "Camera setup ok deviceId=" << deviceId << " size=" << size_.x << "x" << size_.y;
  return true;
}

bool VideoStream::setupCamera(const std::string& deviceName, glm::vec2 desiredSize) {
  stop();

  mode_ = Mode::Camera;

  const auto deviceIdOpt = findUniqueCameraDeviceIdByName(grabber_, deviceName);
  if (!deviceIdOpt) {
    mode_ = Mode::None;
    return false;
  }

  const int deviceId = *deviceIdOpt;

  grabber_.setDeviceID(deviceId);
  grabber_.setPixelFormat(OF_PIXELS_RGB);
  grabber_.setDesiredFrameRate(DEFAULT_VIDEO_FPS);

  const bool ok = grabber_.setup(static_cast<int>(desiredSize.x), static_cast<int>(desiredSize.y));
  if (!ok) {
    ofLogError("VideoStream") << "Failed to setup camera deviceName='" << deviceName << "' deviceId=" << deviceId
                              << " size=" << desiredSize.x << "x" << desiredSize.y;
    mode_ = Mode::None;
    return false;
  }

  allocateFbos(grabber_.getSize());

  ofLogNotice("VideoStream") << "Camera setup ok deviceName='" << deviceName << "' deviceId=" << deviceId
                             << " size=" << size_.x << "x" << size_.y;
  return true;
}

bool VideoStream::setupFile(const std::filesystem::path& path, bool mute, std::optional<int> startPositionSeconds) {
  stop();

  mode_ = Mode::File;

  if (!player_.load(path.string())) {
    ofLogError("VideoStream") << "Failed to load video file: " << path;
    mode_ = Mode::None;
    return false;
  }

  if (mute) {
    player_.setVolume(0.0f);
  }

  player_.play();

  if (startPositionSeconds && *startPositionSeconds > 0) {
    float duration = player_.getDuration();
    if (duration > 0.0f) {
      float position = static_cast<float>(*startPositionSeconds) / duration;
      position = std::clamp(position, 0.0f, 1.0f);
      player_.setPosition(position);
    }
  }

  allocateFbos(player_.getSize());

  ofLogNotice("VideoStream") << "File setup ok path=" << path << " size=" << size_.x << "x" << size_.y;
  return true;
}

void VideoStream::stop() {
  if (player_.isInitialized()) {
    player_.close();
  }
  if (grabber_.isInitialized()) {
    grabber_.close();
  }

  mode_ = Mode::None;
  size_ = { 0.0f, 0.0f };
  frameNewThisUpdate_ = false;
  hasEverReceivedFrame_ = false;
  frameIndex_ = 0;
}

void VideoStream::allocateFbos(glm::vec2 size) {
  size_ = size;

  ofFboSettings s;
  s.width = size_.x;
  s.height = size_.y;
  s.internalformat = GL_RGB8;
  s.useDepth = false;
  s.useStencil = false;
  s.depthStencilAsTexture = false;
  s.numSamples = 0;
  s.minFilter = GL_NEAREST;
  s.maxFilter = GL_NEAREST;
  s.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
  s.wrapModeVertical = GL_CLAMP_TO_EDGE;
  s.textureTarget = GL_TEXTURE_2D;

  frameFbo_.allocate(s);
  frameFbo_.clear(0, 255);
}

void VideoStream::drawTextureToCurrentFrame(const ofTexture& texture, bool mirrorX) {
  frameFbo_.getSource().begin();
  ofEnableBlendMode(OF_BLENDMODE_DISABLED);

  if (mirrorX) {
    texture.draw(texture.getWidth(), 0.0f, -texture.getWidth(), texture.getHeight());
  } else {
    texture.draw(0.0f, 0.0f);
  }

  frameFbo_.getSource().end();
}

void VideoStream::update() {
  frameNewThisUpdate_ = false;

  if (!isAllocated()) {
    return;
  }

  bool hasNewFrame = false;
  if (mode_ == Mode::Camera) {
    if (!grabber_.isInitialized()) {
      return;
    }

    grabber_.update();
    hasNewFrame = grabber_.isFrameNew();
    if (hasNewFrame) {
      frameFbo_.swap();
      drawTextureToCurrentFrame(grabber_.getTexture(), /*mirrorX*/ true);
    }

  } else if (mode_ == Mode::File) {
    if (!player_.isLoaded()) {
      return;
    }

    player_.update();
    hasNewFrame = player_.isFrameNew();
    if (hasNewFrame) {
      frameFbo_.swap();
      drawTextureToCurrentFrame(player_.getTexture(), /*mirrorX*/ false);
    }
  }

  if (hasNewFrame) {
    frameNewThisUpdate_ = true;
    hasEverReceivedFrame_ = true;
    frameIndex_++;
  }
}

} // namespace ofxMarkSynth
