#pragma once

#include "ofMain.h"
#include "ofxMarkSynth.h"
#include "ofxGui.h"

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
	ofxMarkSynth::Synth synth;
  std::unique_ptr<ofxMarkSynth::ModPtrs> createMods();
  ofFbo fbo;
  
  std::shared_ptr<ofxAudioAnalysisClient::LocalGistClient> audioAnalysisClientPtr;
  std::shared_ptr<ofxAudioData::Processor> audioDataProcessorPtr;

  bool guiVisible { true };
  ofxPanel gui;
  ofParameterGroup parameters; // I think we rely on this declaration coming after the synth to ensure that destructors are done in the right order
};
