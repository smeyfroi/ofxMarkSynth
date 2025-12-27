//
//  PixelSnapshotMod.cpp
//  example_collage
//
//  Created by Steve Meyfroidt on 18/05/2025.
//

#include "PixelSnapshotMod.hpp"
#include "core/IntentMapping.hpp"
#include "core/IntentMapper.hpp"



namespace ofxMarkSynth {



PixelSnapshotMod::PixelSnapshotMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config)
: Mod { synthPtr, name, std::move(config) }
{
  sourceNameIdMap = {
    { "SnapshotTexture", SOURCE_SNAPSHOT_TEXTURE }
  };
  
  registerControllerForSource(sizeParameter, sizeController);
}

void PixelSnapshotMod::initParameters() {
  parameters.add(snapshotsPerUpdateParameter);
  parameters.add(sizeParameter);
  parameters.add(agencyFactorParameter);
}

float PixelSnapshotMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

void PixelSnapshotMod::update() {
  syncControllerAgencies();
  sizeController.update();
  
  updateCount += snapshotsPerUpdateParameter;
  if (updateCount >= 1.0) {
    auto drawingLayerPtrOpt = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
    if (!drawingLayerPtrOpt) return;
    auto fboPtr = drawingLayerPtrOpt.value()->fboPtr;
    if (!fboPtr->getSource().isAllocated()) return;

    emit(SOURCE_SNAPSHOT_TEXTURE, createSnapshot(fboPtr->getSource()));
    updateCount = 0;
  }
}

const ofTexture& PixelSnapshotMod::createSnapshot(const ofFbo& sourceFbo) {
  int size = static_cast<int>(sizeController.value);
  if (static_cast<int>(snapshotFbo.getWidth()) != size || static_cast<int>(snapshotFbo.getHeight()) != size) {
    snapshotFbo.allocate(size, size, GL_RGBA8);
  }

  int x = ofRandom(0, sourceFbo.getWidth() - snapshotFbo.getWidth());
  int y = ofRandom(0, sourceFbo.getHeight() - snapshotFbo.getHeight());
  
  snapshotFbo.begin();
  ofClear(0, 0, 0, 0);
  ofPushStyle();
  ofEnableBlendMode(OF_BLENDMODE_DISABLED);  // Direct copy without alpha blending
  ofSetColor(ofFloatColor(1.0, 1.0, 1.0, 1.0));
  sourceFbo.draw(-x, -y);  // Offset to crop
  ofPopStyle();
  snapshotFbo.end();

  return snapshotFbo.getTexture();
}

void PixelSnapshotMod::draw() {
  if (!visible) return;
  ofClear(ofFloatColor(0.0, 0.0, 0.0, 1.0));
  ofPushStyle();
  ofSetColor(ofFloatColor(1.0, 1.0, 1.0, 1.0));
  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  snapshotFbo.draw(0.0, 0.0, 1.0, 1.0);
  ofPopStyle();
}

bool PixelSnapshotMod::keyPressed(int key) {
  if (key == 'X') {
    visible = not visible;
    return true;
  }
  return false;
}

void PixelSnapshotMod::applyIntent(const Intent& intent, float strength) {
  IntentMap im(intent);
  
  // Weighted blend: inverse structure (60%) + granularity (40%)
  float combinedFactor = im.S().inv().get() * 0.6f + im.G().get() * 0.4f;
  float sizeI = linearMap(combinedFactor, sizeController);
  sizeController.updateIntent(sizeI, strength, "(1-S)*.6+G -> lin");
}



} // ofxMarkSynth
