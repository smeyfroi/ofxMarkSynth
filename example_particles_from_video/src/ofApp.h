#pragma once

#include "ofMain.h"
#include "ofxMarkSynth.h"
#include "ofxGui.h"

const std::filesystem::path FFMPEG_BINARY_PATH { "/opt/homebrew/bin/ffmpeg" };


const std::filesystem::path ROOT_SOURCE_MATERIAL_PATH { "/Users/steve/Documents/music-source-material" };

// Audio configuration (Synth-owned audio source)
const std::filesystem::path SOURCE_AUDIO_PATH { ROOT_SOURCE_MATERIAL_PATH/"belfast/20250208-violin-separate-scale-vibrato-harmonics.wav" };
const std::string AUDIO_OUT_DEVICE_NAME = "Apple Inc.: MacBook Pro Speakers";
constexpr int AUDIO_BUFFER_SIZE = 256;
constexpr int AUDIO_CHANNELS = 1;
constexpr int AUDIO_SAMPLE_RATE = 48000;
const std::filesystem::path ROOT_PERFORMANCE_PATH { "/Users/steve/Documents/MarkSynth-performances/Practice" };
const std::filesystem::path PERFORMANCE_CONFIG_ROOT_PATH { ROOT_PERFORMANCE_PATH/"config" }; // must exist
const std::filesystem::path PERFORMANCE_ARTEFACT_ROOT_PATH { ROOT_PERFORMANCE_PATH/"artefact" }; // subdirectories created by Synth

constexpr glm::vec2 COMPOSITE_SIZE { 1080, 1080 }; // Todo: complete this by rippling it through all the examples
constexpr float COMPOSITE_PANEL_GAP_PX = 8.0;
const bool START_PAUSED = false; // false for dev
constexpr float FRAME_RATE = 30.0;
constexpr glm::vec2 VIDEO_RECORDER_SIZE { 1280, 720 };

const std::filesystem::path SOURCE_VIDEO_PATH { ROOT_SOURCE_MATERIAL_PATH/"belfast/trombone-trimmed.mov" };
constexpr bool SOURCE_VIDEO_MUTE = true;
constexpr int CAMERA_DEVICE_ID = 0;
const glm::vec2 VIDEO_SIZE { 640, 480 };
constexpr bool SAVE_RECORDING = false;
const std::filesystem::path VIDEO_RECORDING_PATH { PERFORMANCE_ARTEFACT_ROOT_PATH/"video-recordings" }; // created

class ofApp: public ofBaseApp{
public:
	void setup();
	void update();
	void draw();
	void exit();
	
	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);
	
private:
  std::shared_ptr<ofxMarkSynth::Synth> synthPtr;

  bool guiVisible { true };
  ofxPanel gui;
  ofParameterGroup parameters; // I think we rely on this declaration coming after the synth to ensure that destructors are done in the right order
};
