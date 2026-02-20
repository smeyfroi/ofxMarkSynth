#pragma once

#include "ofMain.h"
#include "ofxMarkSynth.h"

const std::filesystem::path FFMPEG_BINARY_PATH { "/opt/homebrew/bin/ffmpeg" };

const std::filesystem::path ROOT_SOURCE_MATERIAL_PATH { "/Users/steve/Documents/music-source-material" };
const std::filesystem::path ROOT_PERFORMANCE_PATH { "/Users/steve/Documents/MarkSynth-performances/Practice" };

const std::filesystem::path PERFORMANCE_CONFIG_ROOT_PATH { ROOT_PERFORMANCE_PATH/"config" }; // must exist
const std::filesystem::path PERFORMANCE_ARTEFACT_ROOT_PATH { ROOT_PERFORMANCE_PATH/"artefact" }; // subdirectories created by Synth

// Audio configuration (Synth-owned audio source)
const std::filesystem::path SOURCE_AUDIO_PATH { ROOT_SOURCE_MATERIAL_PATH/"cork/audio-2025-06-16-11-25-03-931.wav" };
const std::string AUDIO_OUT_DEVICE_NAME = "Apple Inc.: MacBook Pro Speakers";
// NOTE: bufferSize strongly affects pitch detection range (YIN).
// 2048 @ 44.1kHz supports low trombone fundamentals (~43Hz+).
constexpr int AUDIO_BUFFER_SIZE = 2048;
constexpr int AUDIO_CHANNELS = 1;
constexpr int AUDIO_SAMPLE_RATE = 44100;

constexpr float FRAME_RATE = 30.0f;
constexpr bool START_HIBERNATED = false;
constexpr glm::vec2 COMPOSITE_SIZE = { 768, 768 };
constexpr float COMPOSITE_PANEL_GAP_PX = 8.0f;
constexpr glm::vec2 VIDEO_RECORDER_SIZE { 1280, 720 };

class ofApp: public ofBaseApp{
public:
  void setup() override;
  void update() override;
  void draw() override;
  void exit() override;

  void keyPressed(int key) override;
  void keyReleased(int key) override;
  void windowResized(int w, int h) override;

  void setGuiWindowPtr(std::shared_ptr<ofAppBaseWindow> windowPtr) { guiWindowPtr = windowPtr; }
  void drawGui(ofEventArgs& args);
  void keyPressedEvent(ofKeyEventArgs& e) { keyPressed(e.key); }

private:
  std::shared_ptr<ofxMarkSynth::Synth> synthPtr;
  std::shared_ptr<ofAppBaseWindow> guiWindowPtr;
};
