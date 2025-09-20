#pragma once

#include "ofMain.h"
#include "ofxMarkSynth.h"
#include "ofxGui.h"

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
	
private:
  std::shared_ptr<ofxMarkSynth::Synth> synth;
  ofxMarkSynth::ModPtrs createMods();
  ofxMarkSynth::FboConfigPtrs createFboConfigs();
  ofxMarkSynth::FboPtr fboPtr = std::make_shared<PingPongFbo>();

  bool guiVisible { true };
  ofxPanel gui;
  ofParameterGroup parameters; // I think we rely on this declaration coming after the synth to ensure that destructors are done in the right order

};
