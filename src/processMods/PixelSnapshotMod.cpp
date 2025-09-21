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
  parameters.add(sizeParameter);
}

void PixelSnapshotMod::update() {
  auto fboPtr = fboPtrs[0];
  if (!fboPtr) return;

  updateCount += snapshotsPerUpdateParameter;
  if (updateCount >= 1.0) {
    emit(SOURCE_SNAPSHOT, createSnapshot(fboPtr));
    updateCount = 0;
  }
}

const ofFbo PixelSnapshotMod::createSnapshot(const FboPtr& fboPtr) {
  if (snapshotFbo.getWidth() != sizeParameter || snapshotFbo.getHeight() != sizeParameter) {
    snapshotFbo.allocate(sizeParameter, sizeParameter, GL_RGBA8);
  }

  // This is built for snapshots from a very large source
  int x = ofRandom(0, fboPtr->getWidth() - snapshotFbo.getWidth());
  int y = ofRandom(0, fboPtr->getHeight() - snapshotFbo.getHeight());
  
  snapshotFbo.begin();
  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  ofSetColor(ofFloatColor(1.0, 1.0, 1.0, 1.0));
  fboPtr->getSource().draw(-x, -y);
  snapshotFbo.end();

  return snapshotFbo;
}

void PixelSnapshotMod::draw() {
  if (!visible) return;
  ofClear(ofFloatColor(0.0, 0.0, 0.0, 1.0));
  ofSetColor(ofFloatColor(1.0, 1.0, 1.0, 1.0));
  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  snapshotFbo.draw(0.0, 0.0, 1.0, 1.0);
}

bool PixelSnapshotMod::keyPressed(int key) {
  if (key == 'X') {
    visible = not visible;
    return true;
  }
  return false;
}


} // ofxMarkSynth
