//
//  CollageMod.cpp
//  example_collage
//
//  Created by Steve Meyfroidt on 21/05/2025.
//


#include "CollageMod.hpp"
#include "ofxFatline.h"
#include "IntentMapping.hpp"



namespace ofxMarkSynth {



CollageMod::CollageMod(Synth* synthPtr, const std::string& name, ModConfig config)
: Mod { synthPtr, name, std::move(config) }
{
  sinkNameIdMap = {
    { "path", SINK_PATH },
    { "snapshotFbo", SINK_SNAPSHOT_FBO },
    { colorParameter.getName(), SINK_COLOR }
  };
  
  sourceNameControllerPtrMap = {
    { colorParameter.getName(), &colorController },
    { saturationParameter.getName(), &saturationController },
    { outlineParameter.getName(), &outlineController }
  };
}

void CollageMod::initParameters() {
  parameters.add(strategyParameter);
  parameters.add(colorParameter);
  parameters.add(saturationParameter);
  parameters.add(outlineParameter);
}

void CollageMod::update() {
  colorController.update();
  saturationController.update();
  outlineController.update();
  
  if (path.getCommands().size() <= 3) return;
  if (strategyParameter == 1 && !snapshotFbo.isAllocated()) return;

  auto drawingLayerPtrOpt0 = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt0) return;
  auto drawingLayerPtr0 = drawingLayerPtrOpt0.value();
  auto fboPtr0 = drawingLayerPtr0->fboPtr;
  if (fboPtr0->getSource().getStencilBuffer() == 0) {
    ofLogError("CollageMod") << "CollageMod needs stencil buffer in drawing layer: " << DEFAULT_DRAWING_LAYER_PTR_NAME;
    return;
  }

  auto drawingLayerPtrOpt1 = getCurrentNamedDrawingLayerPtr(OUTLINE_LAYERPTR_NAME);
  float outline = outlineController.value;
  if (outline > 0.0f && drawingLayerPtrOpt1) {
    auto fboPtr1 = drawingLayerPtrOpt1.value()->fboPtr;

    // punch hole through existing outlines
    // TODO: punch hole on fatline as well to avoid the middle seam when the outlines fade. Or add the fatline into the stencil to draw the snapshot only within the fatline interior
    fboPtr1->getSource().begin();
    ofScale(fboPtr1->getWidth(), fboPtr1->getHeight());
    path.setFilled(true);
    ofEnableBlendMode(OF_BLENDMODE_DISABLED);
    path.setColor(ofFloatColor { 0.0, 0.0, 0.0, 0.0 });
    path.draw();

    const float width = 12.0 / fboPtr1->getWidth(); // TODO: parameterise
    const auto& vertices = path.getOutline()[0].getVertices();
    int count = vertices.size();
    std::vector<ofFloatColor> colors(count, ofFloatColor { 0.0, 0.0, 0.0, outline });
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
  
//  // Use ALPHA for layers that clear on update, else SCREEN
//  if (drawingLayerPtr0->clearOnUpdate) ofEnableBlendMode(OF_BLENDMODE_SCREEN);
//  else ofEnableBlendMode(OF_BLENDMODE_ALPHA);

  ofFloatColor tintColor = ofFloatColor(1.0, 1.0, 1.0, 1.0);
  if (strategyParameter != 2) { // 2 == draw an untinted snapshot
    tintColor = colorController.value; // comes from a connected palette or manually
    float currentSaturation = tintColor.getSaturation();
    tintColor.setSaturation(std::clamp(currentSaturation * saturationController.value, 0.0f, 1.0f));
  }
  
  ofEnableBlendMode(OF_BLENDMODE_SCREEN);
  
  if (strategyParameter == 0) {
    path.setFilled(true);
    path.setColor(tintColor);
    path.draw();
  } else {
    glEnable(GL_STENCIL_TEST);
    
    // draw stencil as mask (1s inside path)
    glClear(GL_STENCIL_BUFFER_BIT);
    glStencilFunc(GL_ALWAYS, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glColorMask(false, false, false, false);
    path.setFilled(true);
    path.draw();
    
    // Now only draw snapshot where stencil is 1
    glStencilFunc(GL_EQUAL, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glColorMask(true, true, true, true);
    
    ofSetColor(tintColor);

    ofRectangle normalisedPathBounds { path.getOutline()[0].getBoundingBox() };
    float x = normalisedPathBounds.x;
    float y = normalisedPathBounds.y;
    float w = normalisedPathBounds.width;
    float h = normalisedPathBounds.height;
    
    // Could also limit the scaling to some limit, and optionally crop here?
    
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
      ofLogError("CollageMod") << "ofFbo receive for unknown sinkId " << sinkId;
  }
}

void CollageMod::receive(int sinkId, const ofPath& path_) {
  switch (sinkId) {
    case SINK_PATH:
      path = path_;
      break;
    default:
      ofLogError("CollageMod") << "ofPath receive for unknown sinkId " << sinkId;
  }
}

void CollageMod::receive(int sinkId, const glm::vec4& v) {
  switch (sinkId) {
    case SINK_COLOR:
      colorController.updateAuto(ofFloatColor { v.r, v.g, v.b, v.a }, getAgency());
      break;
    default:
      ofLogError("CollageMod") << "glm::vec4 receive for unknown sinkId " << sinkId;
  }
}

void CollageMod::applyIntent(const Intent& intent, float strength) {
  if (strength < 0.01) return;

  ofFloatColor energetic = energyToColor(intent);
  ofFloatColor structured = structureToBrightness(intent);
  ofFloatColor mixed = energetic.getLerped(structured, 0.25f);
  ofFloatColor finalColor = densityToAlpha(intent, mixed);
  colorController.updateIntent(finalColor, strength);

  float satEnergy = linearMap(intent.getEnergy(), 0.8f, 2.2f);
  float satChaos = exponentialMap(intent.getChaos(), 0.9f, 2.8f, 2.0f);
  float satStructure = inverseMap(intent.getStructure(), 0.8f, 1.6f);
  float targetSaturation = std::clamp(satEnergy * satChaos * satStructure, 0.0f, 3.0f);
  saturationController.updateIntent(targetSaturation, strength);

//  if (strength > 0.01f) {
//    int strategy = (intent.getStructure() > 0.55f || intent.getGranularity() > 0.6f) ? 1 : 0;
//    if (strategyParameter.get() != strategy) strategyParameter.set(strategy);
//    bool outlineOn = intent.getChaos() < 0.7f;
//    if (outlineParameter.get() != outlineOn) outlineParameter.set(outlineOn);
//  }
}



} // ofxMarkSynth
