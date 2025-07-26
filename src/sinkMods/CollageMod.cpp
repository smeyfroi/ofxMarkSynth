//
//  CollageMod.cpp
//  example_collage
//
//  Created by Steve Meyfroidt on 21/05/2025.
//


#include "CollageMod.hpp"
#include "AddTextureThresholded.h"


namespace ofxMarkSynth {


CollageMod::CollageMod(const std::string& name, const ModConfig&& config)
: Mod { name, std::move(config) }
{
  maskShader.load();
  addTextureThresholdedShader.load();
}

void CollageMod::initParameters() {
  parameters.add(colorParameter);
  parameters.add(strengthParameter);
}

void CollageMod::initTempFbo() {
  if (! tempFbo.isAllocated()) {
    auto fboPtr = fboPtrs[0];
    ofFboSettings settings { nullptr };
    settings.wrapModeVertical = GL_REPEAT;
    settings.wrapModeHorizontal = GL_REPEAT;
    settings.width = fboPtr->getWidth();
    settings.height = fboPtr->getHeight();
    settings.internalformat = FLOAT_A_MODE;
    settings.numSamples = 0;
    settings.useDepth = true;
    settings.useStencil = true;
    settings.textureTarget = ofGetUsingArbTex() ? GL_TEXTURE_RECTANGLE_ARB : GL_TEXTURE_2D;
    tempFbo.allocate(settings);
  }
}

void CollageMod::update() {
  if (path.getCommands().size() <= 2) return;
  if (! collageSourceTexture.isAllocated()) return;

  auto fboPtr = fboPtrs[0];
  if (!fboPtr) return;
  
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
//  constexpr float MAX_SCALE = 3.0;
//  float scaleX = std::fminf(MAX_SCALE, 1.0 / pathBounds.width);
//  float scaleY = std::fminf(MAX_SCALE, 1.0 / pathBounds.height);
//  float scale = std::fminf(scaleX, scaleY);

  // scale to fit the incoming pixels in the path bounds
  float scaleX = 1.0 / pathBounds.width;
  float scaleY = 1.0 / pathBounds.height;
  float scale = std::fminf(scaleX, scaleY);

  // draw scaled, coloured pixels into a temporary FBO through the mask using a blend mode
  initTempFbo();
  tempFbo.begin();
  {
    ofClear(0, 0, 0, 0);
    ofEnableBlendMode(OF_BLENDMODE_ADD);
//    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    ofFloatColor c = colorParameter;
    c *= strengthParameter; c.a *= strengthParameter;
    ofSetColor(c);
    maskShader.render(collageSourceTexture, maskFbo,
                      tempFbo.getWidth(), tempFbo.getHeight(),
                      false,
                      {pathBounds.x+pathBounds.width/2.0, pathBounds.y+pathBounds.height/2.0},
                      {scale, scale});
  }
  tempFbo.end();
  
  // Then composite the collage FBO onto the target with a threshold
  addTextureThresholdedShader.render(*fboPtr, tempFbo);

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

//void CollageMod::draw() {
//  tempFbo.draw(0, 0);
//}

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
