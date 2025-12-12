#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "ofMain.h"
#include "ofxMarkSynth.h"
#include "ofxGui.h"

const std::filesystem::path ROOT_SOURCE_MATERIAL_PATH { "/Users/steve/Documents/music-source-material" };
const std::filesystem::path SOURCE_AUDIO_PATH { ROOT_SOURCE_MATERIAL_PATH/"belfast/20250208-violin-separate-scale-vibrato-harmonics.wav" };
const std::string AUDIO_OUT_DEVICE_NAME = "Apple Inc.: MacBook Pro Speakers";
constexpr int AUDIO_BUFFER_SIZE = 256;
constexpr int AUDIO_CHANNELS = 1;
constexpr int AUDIO_SAMPLE_RATE = 48000;
constexpr float FRAME_RATE = 30.0f;
constexpr bool START_PAUSED = false;

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
};
