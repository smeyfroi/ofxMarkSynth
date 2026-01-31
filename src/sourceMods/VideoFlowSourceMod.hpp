//
//  VideoFlowSourceMod.hpp
//  example_particles_from_video
//
//  Created by Steve Meyfroidt on 09/05/2025.
//

#pragma once

#include <filesystem>
#include <string>
#include "core/Mod.hpp"
#include "core/ParamController.h"
#include "ofxMotionFromVideo.h"
#ifdef TARGET_MAC
#include "ofxFFmpegRecorder.h"
#endif



namespace ofxMarkSynth {



class VideoFlowSourceMod : public Mod {

public:
  VideoFlowSourceMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config, const std::filesystem::path& sourceVideoFilePath, bool mute, const std::string& startPosition = "");
  VideoFlowSourceMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config, int deviceID, glm::vec2 size, bool saveRecording, const std::filesystem::path& recordingPath);
  ~VideoFlowSourceMod();
  void shutdown() override;
  float getAgency() const override;
  void update() override;
  void draw() override;
  bool keyPressed(int key) override;
  void applyIntent(const Intent& intent, float strength) override;

  struct MotionSampleStats {
    int samplesAttempted { 0 };
    int samplesAccepted { 0 };
    float acceptRate { 0.0f };
    float acceptedSpeedMean { 0.0f };
    float acceptedSpeedMax { 0.0f };
    bool cpuSamplingEnabled { false };
  };

  MotionSampleStats getMotionSampleStats() const { return motionSampleStats; }
  bool isMotionReady() const { return motionFromVideo.isReady(); }
  const ofFbo& getVideoFbo() const { return motionFromVideo.getVideoFbo(); }
  const ofFbo& getMotionFbo() const { return motionFromVideo.getMotionFbo(); }

  UiState captureUiState() const override {
    UiState state;
    setUiStateBool(state, "videoVisible", motionFromVideo.isVideoVisible());
    setUiStateBool(state, "motionVisible", motionFromVideo.isMotionVisible());
    return state;
  }

  void restoreUiState(const UiState& state) override {
    bool defaultVideoVisible = motionFromVideo.isVideoVisible();
    motionFromVideo.setVideoVisible(getUiStateBool(state, "videoVisible", defaultVideoVisible));

    bool defaultMotionVisible = motionFromVideo.isMotionVisible();
    motionFromVideo.setMotionVisible(getUiStateBool(state, "motionVisible", defaultMotionVisible));
  }

  static constexpr int SOURCE_FLOW_FIELD = 10;
  static constexpr int SOURCE_POINT_VELOCITY = 20;
  static constexpr int SOURCE_POINT = 21;

protected:
  void initParameters() override;

private:
  MotionFromVideo motionFromVideo;

  // Default tuned from performance configs: 140 is a good general baseline.
  ofParameter<float> pointSamplesPerUpdateParameter { "PointSamplesPerUpdate", 140.0f, 0.0f, 500.0f };
  ParamController<float> pointSamplesPerUpdateController { pointSamplesPerUpdateParameter };

  // Retry budget for intermittent acceptance. Keeps sampling uniformly random across the frame,
  // but increases the chance of hitting moving regions.
  ofParameter<float> pointSampleAttemptMultiplierParameter { "PointSampleAttemptMultiplier", 1.0f, 1.0f, 20.0f };

  ofParameter<float> agencyFactorParameter { "AgencyFactor", 1.0f, 0.0f, 1.0f };

  MotionSampleStats motionSampleStats;

  bool saveRecording;
  std::string recordingDir;
#ifdef TARGET_MAC
  ofxFFmpegRecorder recorder;
  void initRecorder();
#endif
};



} // ofxMarkSynth
