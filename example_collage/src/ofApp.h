#pragma once

#include "ofMain.h"
#include "ofxMarkSynth.h"
#include "ofxGui.h"

constexpr float FRAME_RATE = 30.0;
const bool START_PAUSED = false; // false for dev
const glm::vec2 SYNTH_COMPOSITE_SIZE = { 1200, 1200 }; // drawing layers are scaled down to this size to fit into the window height

class ofApp: public ofBaseApp{
public:
	void setup() override;
	void update() override;
	void draw() override;
	void exit() override;
	
	void keyPressed(int key) override;
	void keyReleased(int key) override;
	void mouseMoved(int x, int y) override;
	void mouseDragged(int x, int y, int button) override;
	void mousePressed(int x, int y, int button) override;
	void mouseReleased(int x, int y, int button) override;
	void windowResized(int w, int h) override;
	void dragEvent(ofDragInfo dragInfo) override;
	void gotMessage(ofMessage msg) override;

  // >>> imgui
  void setGuiWindowPtr(std::shared_ptr<ofAppBaseWindow> windowPtr) { guiWindowPtr = windowPtr; }
  void drawGui(ofEventArgs& args);
  void keyPressedEvent(ofKeyEventArgs& e) { keyPressed(e.key); } // adapter for ofAddListener
  // <<< imgui

private:
  std::shared_ptr<ofxMarkSynth::Synth> synthPtr;
  
  // >>> imgui
  std::shared_ptr<ofAppBaseWindow> guiWindowPtr;
  // <<< imgui
};
