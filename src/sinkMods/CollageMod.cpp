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
{
  maskShader.load();
}

void CollageMod::initParameters() {
  parameters.add(colorParameter);
  parameters.add(strengthParameter);
}

void CollageMod::update() {
  if (path.getCommands().size() <= 2) return;
  if (! collageSourceTexture.isAllocated()) return;

  auto fboPtr = fboPtrs[0];
  if (!fboPtr) return;

  // TODO: check a dirty flag? Or let the collage build up over multiple updates as now?
  
  // Make a mask texture from the normalised path
  ofFbo maskFbo;
  maskFbo.allocate(fboPtr->getWidth(), fboPtr->getHeight());
  maskFbo.begin();
  {
    ofPath maskPath = path;
    ofEnableBlendMode(OF_BLENDMODE_DISABLED);
    ofClear(0, 255);
    ofSetColor(255);
    maskPath.setFilled(true);
    maskPath.scale(maskFbo.getWidth(), maskFbo.getHeight());
    maskPath.draw();
  }
  maskFbo.end();

  // find normalised path bounds
  ofRectangle pathBounds;
  for (const auto& polyline : path.getOutline()) {
    pathBounds = pathBounds.getUnion(polyline.getBoundingBox());
  }

  // find a proportional scale to some limit to fill the mask with a reduced view of part of the pixels
  constexpr float MAX_SCALE = 3.0;
  float scaleX = std::fminf(MAX_SCALE, 1.0 / pathBounds.width);
  float scaleY = std::fminf(MAX_SCALE, 1.0 / pathBounds.height);
  float scale = std::fminf(scaleX, scaleY);

  // draw scaled, coloured pixels into the FBO through the mask using a blend mode
  fboPtr->getSource().begin();
  {
    ofEnableBlendMode(OF_BLENDMODE_ADD);
//    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    ofFloatColor c = colorParameter;
    c -= 0.5; c *= strengthParameter; c.a *= strengthParameter;
//    c *= strengthParameter; c.a *= strengthParameter;
    ofSetColor(c);
    maskShader.render(collageSourceTexture, maskFbo,
                      fboPtr->getWidth(), fboPtr->getHeight(),
                      false,
                      {pathBounds.x+pathBounds.width/2.0, pathBounds.y+pathBounds.height/2.0},
                      {scale, scale});
  }
  fboPtr->getSource().end();
  
// THIS IS WRONG: feed points forward into a DividedArea instead
  // draw outline on path
//  fboPtr->getSource().begin();
//  {
//    ofScale(fboPtr->getWidth(), fboPtr->getHeight());
//    path.setStrokeColor({0.0, 0.0, 0.0, 0.0});
//    path.setStrokeWidth(1.0);
//    path.draw();
//  }
//  fboPtr->getSource().end();
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
