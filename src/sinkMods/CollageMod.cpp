//
//  CollageMod.cpp
//  example_collage
//
//  Created by Steve Meyfroidt on 21/05/2025.
//


#include "CollageMod.hpp"


namespace ofxMarkSynth {


CollageMod::CollageMod(const std::string& name, const ModConfig&& config)
: Mod { name, std::move(config) }
{}

void CollageMod::initParameters() {
  parameters.add(strategyParameter);
  parameters.add(colorParameter);
  parameters.add(strengthParameter);
}

void CollageMod::update() {
  if (path.getCommands().size() <= 2) return;
  if (strategyParameter == 1 && !collageSourceTexture.isAllocated()) return;

  auto fboPtr = fboPtrs[0];
  if (!fboPtr) return;
  
  fboPtr->getSource().begin();
  ofScale(fboPtr->getWidth(), fboPtr->getHeight());

  if (strategyParameter == 0) {
    // tint
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    ofFloatColor c = colorParameter;
    c *= strengthParameter; c.a *= strengthParameter;
    path.setFilled(true);
    path.setColor(c);
    path.draw();
    
  } else if (strategyParameter == 1) {
    // paste tinted
    glEnable(GL_STENCIL_TEST);
    glClear(GL_STENCIL_BUFFER_BIT);

    // Setup stencil: write 1s where path is drawn
    glStencilFunc(GL_ALWAYS, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glColorMask(false, false, false, false);

    path.setFilled(true);
    path.draw();

    // Now only draw where stencil is 1
    glStencilFunc(GL_EQUAL, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glColorMask(true, true, true, true);

    ofEnableBlendMode(OF_BLENDMODE_ADD);
    ofFloatColor c = colorParameter;
    c *= strengthParameter; c.a *= strengthParameter;
    ofSetColor(c);
//    ofSetColor(255);
    
    ofRectangle normalisedPathBounds { path.getOutline()[0].getBoundingBox() };
//    for (const auto& polyline : path.getOutline()) {
//      normalisedPathBounds = normalisedPathBounds.getUnion(polyline.getBoundingBox());
//    }
    float x = normalisedPathBounds.x;
    float y = normalisedPathBounds.y;
    float w = normalisedPathBounds.width;
    float h = normalisedPathBounds.height;

    collageSourceTexture.draw(x, y, w, h);

    glDisable(GL_STENCIL_TEST);
  }
  fboPtr->getSource().end();

  path.clear();
}

void CollageMod::receive(int sinkId, const ofFloatPixels& pixels) {
  switch (sinkId) {
    case SINK_PIXELS:
      collageSourceTexture.allocate(pixels);
      break;
    default:
      ofLogError() << "ofPixels receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void CollageMod::receive(int sinkId, const ofPath& path_) {
  switch (sinkId) {
    case SINK_PATH:
      path = path_;
      break;
    default:
      ofLogError() << "ofPath receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void CollageMod::receive(int sinkId, const glm::vec4& v) {
  switch (sinkId) {
    case SINK_COLOR:
      colorParameter = ofFloatColor { v.r, v.g, v.b, v.a };
      break;
    default:
      ofLogError() << "glm::vec4 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}


} // ofxMarkSynth
