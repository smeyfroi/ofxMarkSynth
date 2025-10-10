//
//  CollageMod.cpp
//  example_collage
//
//  Created by Steve Meyfroidt on 21/05/2025.
//


#include "CollageMod.hpp"
#include "ofxFatline.h"


namespace ofxMarkSynth {


CollageMod::CollageMod(const std::string& name, const ModConfig&& config)
: Mod { name, std::move(config) }
{}

void CollageMod::initParameters() {
  parameters.add(strategyParameter);
  parameters.add(colorParameter);
  parameters.add(strengthParameter);
  parameters.add(outlineParameter);
}

void CollageMod::update() {
  if (path.getCommands().size() <= 3) return;
  if (strategyParameter == 1 && !snapshotFbo.isAllocated()) return;

  auto fboPtrOpt0 = getNamedFboPtr(DEFAULT_FBOPTR_NAME);
  if (!fboPtrOpt0) return;
  auto fboPtr0 = fboPtrOpt0.value();

  auto fboPtrOpt1 = getNamedFboPtr(OUTLINE_FBOPTR_NAME);
  if (outlineParameter && fboPtrOpt1) {
    auto fboPtr1 = fboPtrOpt1.value();

    // punch hole through existing outlines
    fboPtr1->getSource().begin();
    ofScale(fboPtr1->getWidth(), fboPtr1->getHeight());
    path.setFilled(true);
    ofEnableBlendMode(OF_BLENDMODE_DISABLED);
    path.setColor(ofFloatColor { 0.0, 0.0, 0.0, 0.0 });
    path.draw();

    const float width = 12.0 / fboPtr1->getWidth(); // TODO: parameterise
    const auto& vertices = path.getOutline()[0].getVertices();
    int count = vertices.size();
    std::vector<ofFloatColor> colors(count, ofFloatColor { 0.0, 0.0, 0.0, 1.0 });
    std::vector<double> widths(count, width);
    ofxFatLine fatline;
    fatline.setFeather(width/8.0); // NOTE: getting this wrong can cause weird artifacts
//    fatline.setJointType(OFX_FATLINE_JOINT_ROUND); // causes weird artefacts?
//    fatline.setCapType(OFX_FATLINE_CAP_SQUARE); // causes weird artefacts?
    fatline.add(vertices, colors, widths);
    fatline.add(vertices.front(), colors.front(), widths.front()); // close the loop
    
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    fatline.draw();
    fboPtr1->getSource().end();
  }

  fboPtr0->getSource().begin();
  ofScale(fboPtr0->getWidth(), fboPtr0->getHeight());

  // Close the path for drawing the fill
  path.close();
  
  if (strategyParameter == 0) {
    // tint
    ofEnableBlendMode(OF_BLENDMODE_DISABLED); // reset first
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    ofFloatColor c = colorParameter;
    c *= strengthParameter; c.a *= strengthParameter;
    
    path.setFilled(true);
    path.setColor(c);
    path.draw();
    
  } else {
    ofFloatColor c;
    if (strategyParameter == 1) {
      c = colorParameter;
    } else {
      c = ofFloatColor(1.0, 1.0, 1.0, 1.0);
    }
    c *= strengthParameter; c.a *= strengthParameter;
    
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
    
    ofEnableBlendMode(OF_BLENDMODE_DISABLED); // reset first
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    ofSetColor(c);
    ofRectangle normalisedPathBounds { path.getOutline()[0].getBoundingBox() };
    float x = normalisedPathBounds.x;
    float y = normalisedPathBounds.y;
    float w = normalisedPathBounds.width;
    float h = normalisedPathBounds.height;
    
    // TODO: limit the scaling to some limit, and optionally crop
    
    snapshotFbo.draw(x, y, w, h);
    
    glDisable(GL_STENCIL_TEST);
  }

  fboPtr0->getSource().end();
  path.clear();
}

void CollageMod::receive(int sinkId, const ofFbo& value) {
  switch (sinkId) {
    case SINK_SNAPSHOT_FBO:
      //  ofxMarkSynth::fboCopyBlit(newFieldFbo, fieldFbo);
      //  ofxMarkSynth::fboCopyDraw(newFieldFbo, fieldFbo);
      snapshotFbo = value;
      break;
    default:
      ofLogError() << "ofFbo receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
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
