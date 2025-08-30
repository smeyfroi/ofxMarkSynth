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
    emit(SOURCE_PIXELS, createPixels(fboPtr));
    updateCount = 0;
  }
}

const ofFloatPixels PixelSnapshotMod::createPixels(const FboPtr& fboPtr) {
  auto format = fboPtr->getSource().getTexture().getTextureData().glInternalFormat;
  if (!pixels.isAllocated() || pixels.getWidth() != sizeParameter || pixels.getHeight() != sizeParameter) {
    pixels.allocate(sizeParameter, sizeParameter, ofGetImageTypeFromGLType(format));
  }

  int x = ofRandom(0, fboPtr->getWidth() - pixels.getWidth());
  int y = ofRandom(0, fboPtr->getHeight() - pixels.getHeight());
  
  fboPtr->getSource().bind();
  glReadPixels(x, y, pixels.getWidth(), pixels.getHeight(), GL_RGBA/*format*/, GL_FLOAT, pixels.getData()); // FIXME: assumes float RGBA
  fboPtr->getSource().unbind();

  return pixels;
}

void PixelSnapshotMod::draw() {
  if (!visible) return;
  {
    ofSetColor(ofFloatColor(0.2, 0.2, 0.2, 1.0));
    ofFill();
    ofDrawRectangle(0.0, 0.0, 1.0, 1.0);
  }
  {
    ofTexture t;
    t.allocate(pixels.getWidth(), pixels.getHeight(), GL_RGBA);
    t.loadData(pixels);
    ofSetColor(ofFloatColor(1.0, 1.0, 1.0, 1.0));
    t.draw({ 0, 0 }, 1.0, 1.0);
  }
}

bool PixelSnapshotMod::keyPressed(int key) {
  if (key == 'X') {
    visible = not visible;
    return true;
  }
  return false;
}


} // ofxMarkSynth
