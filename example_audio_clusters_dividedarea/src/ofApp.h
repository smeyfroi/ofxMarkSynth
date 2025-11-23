#pragma once

#include "ofMain.h"
#include "ofxMarkSynth.h"
#include "ofxGui.h"

const std::filesystem::path ROOT_SOURCE_MATERIAL_PATH { "/Users/steve/Documents/music-source-material" };
const std::filesystem::path SOURCE_AUDIO_PATH { ROOT_SOURCE_MATERIAL_PATH/"belfast/20250208-violin-separate-scale-vibrato-harmonics.wav" };
constexpr bool RECORD_AUDIO = false;
const std::string MIC_DEVICE_NAME = "Apple Inc.: MacBook Pro Microphone";
constexpr float FRAME_RATE = 30.0;
constexpr bool START_PAUSED = false; // false for dev
constexpr glm::vec2 SYNTH_COMPOSITE_SIZE = { 768, 768 }; // drawing layers are scaled down to this size to fit into the window height

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
