//
//  PixelSnapshotMod.cpp
//  example_collage
//
//  Created by Steve Meyfroidt on 18/05/2025.
//

#include "PixelSnapshotMod.hpp"
#include "IntentMapping.hpp"



namespace ofxMarkSynth {



PixelSnapshotMod::PixelSnapshotMod(Synth* synthPtr, const std::string& name, const ModConfig&& config)
: Mod { synthPtr, name, std::move(config) }
{
  sinkNameIdMap = {
    { "snapshotSource", SINK_SNAPSHOT_SOURCE }
  };
  sourceNameIdMap = {
    { "snapshot", SOURCE_SNAPSHOT }
  };
}

void PixelSnapshotMod::initParameters() {
  parameters.add(snapshotsPerUpdateParameter);
  parameters.add(sizeParameter);
}

void PixelSnapshotMod::update() {
  if (!sourceFbo.isAllocated()) return;

  sizeController.update();
  
  updateCount += snapshotsPerUpdateParameter;
  if (updateCount >= 1.0) {
    emit(SOURCE_SNAPSHOT, createSnapshot(sourceFbo));
    updateCount = 0;
  }
}

const ofFbo PixelSnapshotMod::createSnapshot(const ofFbo& fbo) {
  int size = static_cast<int>(sizeController.value);
  if (static_cast<int>(snapshotFbo.getWidth()) != size || static_cast<int>(snapshotFbo.getHeight()) != size) {
    snapshotFbo.allocate(size, size, GL_RGBA8);
  }

  int x = ofRandom(0, fbo.getWidth() - snapshotFbo.getWidth());
  int y = ofRandom(0, fbo.getHeight() - snapshotFbo.getHeight());
  
  snapshotFbo.begin();
  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  ofSetColor(ofFloatColor(1.0, 1.0, 1.0, 1.0));
  fbo.draw(-x, -y);
  snapshotFbo.end();

  return snapshotFbo;
}

void PixelSnapshotMod::receive(int sinkId, const ofFbo& value) {
  switch (sinkId) {
    case SINK_SNAPSHOT_SOURCE:
      //  ofxMarkSynth::fboCopyBlit(newFieldFbo, fieldFbo);
      //  ofxMarkSynth::fboCopyDraw(newFieldFbo, fieldFbo);
      sourceFbo = value;
      break;
    default:
      ofLogError() << "ofFbo receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
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

void PixelSnapshotMod::applyIntent(const Intent& intent, float strength) {
  if (strength < 0.01) return;
  
  // Inverse Structure → Size
  float sizeI = inverseMap(intent.getStructure(), sizeController);
  sizeController.updateIntent(sizeI, strength);
}



} // ofxMarkSynth
