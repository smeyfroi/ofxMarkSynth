#pragma once

#include "ofMain.h"
#include "ofxMarkSynth.h"
#include "ofxGui.h"

const std::filesystem::path ROOT_SOURCE_MATERIAL_PATH { "/Users/steve/Documents/music-source-material" };
const std::filesystem::path SOURCE_VIDEO_PATH { ROOT_SOURCE_MATERIAL_PATH/"belfast/trombone-trimmed.mov" };
constexpr bool SOURCE_VIDEO_MUTE = false;
const std::filesystem::path SOURCE_AUDIO_PATH { ROOT_SOURCE_MATERIAL_PATH/"belfast/20250208-violin-separate-scale-vibrato-harmonics.wav" };
constexpr int VIDEO_DEVICE_ID = 0;
constexpr bool RECORD_VIDEO = false;
constexpr bool RECORD_AUDIO = false;
const std::string MIC_DEVICE_NAME = "Apple Inc.: MacBook Pro Microphone";
constexpr float FRAME_RATE = 30.0;
constexpr bool START_PAUSED = false; // false for dev
const std::string MAX_RMS = "0.02"; // "0.02"; // "0.11" more likely for live
const std::filesystem::path RECORDING_PATH { "/Users/steve/Documents/recordings" };
constexpr glm::vec2 SYNTH_COMPOSITE_SIZE = { 1080, 1080 }; // drawing layers are scaled down to this size to fit into the window height

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
  void onSynthHibernationComplete(ofxMarkSynth::Synth::HibernationCompleteEvent& e);
	
private:
  std::shared_ptr<ofxMarkSynth::Synth> synthPtr;

  bool guiVisible { true };
  ofxPanel gui;
  ofParameterGroup parameters; // I think we rely on this declaration coming after the synth to ensure that destructors are done in the right order
};
