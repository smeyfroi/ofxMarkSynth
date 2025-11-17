#pragma once

#include "ofMain.h"
#include "ofxMarkSynth.h"
#include "ofxGui.h"

constexpr float FRAME_RATE = 30.0;
const bool START_PAUSED = false; // false for dev
const glm::vec2 SYNTH_COMPOSITE_SIZE = { 1080, 1080 }; // drawing layers are scaled down to this size to fit into the window height

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
