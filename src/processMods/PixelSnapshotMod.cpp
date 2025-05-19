//
//  PixelSnapshotMod.cpp
//  example_collage
//
//  Created by Steve Meyfroidt on 18/05/2025.
//

#include "PixelSnapshotMod.hpp"


namespace ofxMarkSynth {


PixelSnapshotMod::PixelSnapshotMod(const std::string& name, const ModConfig&& config)
: Mod { name, std::move(config) }
{}

void PixelSnapshotMod::initParameters() {
  parameters.add(snapshotsPerUpdateParameter);
}

void PixelSnapshotMod::update() {
  auto fboPtr = fboPtrs[0];
  if (!fboPtr) return;

  updateCount += snapshotsPerUpdateParameter;
  if (updateCount >= 1.0) {
    emit(SOURCE_PIXELS, createPixels(fboPtr));
    updateCount = 0;
  }
}

const ofPixels PixelSnapshotMod::createPixels(const FboPtr& fboPtr) {
  fboPtr->getSource().readToPixels(pixels);
  return pixels;
}

void PixelSnapshotMod::draw() {
  if (!visible) return;
  ofPushStyle();
  {
    // clear whatever is drawn on the window already
    ofSetColor(ofFloatColor(0.0, 0.0, 0.0, 1.0));
    ofFill();
    ofDrawRectangle(0.0, 0.0, ofGetWindowWidth(), ofGetWindowHeight());
  }
  {
    ofTexture t;
    t.allocate(pixels.getWidth(), pixels.getHeight(), GL_RGBA);
    t.loadData(pixels);
    ofSetColor(ofFloatColor(1.0, 1.0, 1.0, 1.0));
    t.draw({ 0, 0 }, ofGetWindowWidth(), ofGetWindowHeight());
  }
  ofPopStyle();
}

bool PixelSnapshotMod::keyPressed(int key) {
  if (key == 'X') {
    visible = not visible;
    return true;
  }
  return false;
}


} // ofxMarkSynth
