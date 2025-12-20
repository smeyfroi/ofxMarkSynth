//
//  CollageMod.cpp
//  example_collage
//
//  Created by Steve Meyfroidt on 21/05/2025.
//


#include "CollageMod.hpp"
#include "ofxFatline.h"
#include "IntentMapping.hpp"
#include "../IntentMapper.hpp"



namespace ofxMarkSynth {



CollageMod::CollageMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config)
: Mod { synthPtr, name, std::move(config) }
{
  sinkNameIdMap = {
    { "Path", SINK_PATH },
    { "SnapshotTexture", SINK_SNAPSHOT_TEXTURE },
    { colorParameter.getName(), SINK_COLOR }
  };
  
  registerControllerForSource(colorParameter, colorController);
  registerControllerForSource(saturationParameter, saturationController);
  registerControllerForSource(outlineAlphaFactorParameter, outlineAlphaFactorController);
  registerControllerForSource(outlineWidthParameter, outlineWidthController);
  registerControllerForSource(outlineColorParameter, outlineColorController);
}

void CollageMod::initParameters() {
  parameters.add(strategyParameter);
  parameters.add(colorParameter);
  parameters.add(saturationParameter);
  parameters.add(outlineAlphaFactorParameter);
  parameters.add(outlineWidthParameter);
  parameters.add(outlineColorParameter);
  parameters.add(agencyFactorParameter);
}

float CollageMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

void CollageMod::drawOutline(std::shared_ptr<PingPongFbo> fboPtr, float outlineAlphaFactor) {
  // Punch hole through existing outlines and draw fatline border
  // TODO: punch hole on fatline as well to avoid the middle seam when the outlines fade.
  // Or add the fatline into the stencil to draw the snapshot only within the fatline interior
  fboPtr->getSource().begin();
  ofPushStyle();
  ofScale(fboPtr->getWidth(), fboPtr->getHeight());

  // Clear interior of path
  path.setFilled(true);
  ofEnableBlendMode(OF_BLENDMODE_DISABLED);
  path.setColor(ofFloatColor { 0.0, 0.0, 0.0, 0.0 });
  path.draw();

  // Draw fatline outline using parameterized width and color
  const float width = outlineWidthController.value / fboPtr->getWidth();
  const auto& vertices = path.getOutline()[0].getVertices();
  int count = vertices.size();
  ofFloatColor outlineColor = outlineColorController.value;
  outlineColor.a *= outlineAlphaFactor; // modulate alpha by outline alpha factor for fade effect
  std::vector<ofFloatColor> colors(count, outlineColor);
  std::vector<double> widths(count, width);
  ofxFatLine fatline;
  fatline.setFeather(width / 8.0); // NOTE: getting this wrong can cause weird artifacts
  // fatline.setJointType(OFX_FATLINE_JOINT_ROUND); // causes weird artefacts?
  // fatline.setCapType(OFX_FATLINE_CAP_SQUARE); // causes weird artefacts?
  fatline.add(vertices, colors, widths);
  fatline.add(vertices.front(), colors.front(), widths.front()); // close the loop

  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  fatline.draw();
  ofPopStyle();
  fboPtr->getSource().end();
}

void CollageMod::drawStrategyTintFill(const ofFloatColor& tintColor) {
  // Strategy 0: Simple tinted fill of the path
  path.setFilled(true);
  path.setColor(tintColor);
  path.draw();
}

void CollageMod::drawStrategySnapshot(const ofFloatColor& tintColor) {
  // Strategy 1 & 2: Draw snapshot texture clipped to path using stencil buffer
  glEnable(GL_STENCIL_TEST);

  ofLogVerbose() << "CollageMod drawing at frame " << ofGetFrameNum() << " with outlineAlphaFactor " << outlineAlphaFactorController.value;

  // Draw stencil as mask (1s inside path)
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

  snapshotTexture.draw(x, y, w, h);

  glDisable(GL_STENCIL_TEST);
}

void CollageMod::update() {
  syncControllerAgencies();
  colorController.update();
  saturationController.update();
  outlineAlphaFactorController.update();
  outlineWidthController.update();
  outlineColorController.update();

  if (path.getCommands().size() <= 3) return;
  if (strategyParameter != 0 && !snapshotTexture.isAllocated()) return;

  auto drawingLayerPtrOpt0 = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt0) return;
  auto drawingLayerPtr0 = drawingLayerPtrOpt0.value();
  auto fboPtr0 = drawingLayerPtr0->fboPtr;
  if (fboPtr0->getSource().getStencilBuffer() == 0) {
    ofLogError("CollageMod") << "CollageMod needs stencil buffer in drawing layer: " << DEFAULT_DRAWING_LAYER_PTR_NAME;
    return;
  }

  // Draw outline if enabled and outline layer exists
  float outlineAlphaFactor = outlineAlphaFactorController.value;
  auto drawingLayerPtrOpt1 = getCurrentNamedDrawingLayerPtr(OUTLINE_LAYERPTR_NAME);
  if (outlineAlphaFactor > 0.0f && drawingLayerPtrOpt1) {
    drawOutline(drawingLayerPtrOpt1.value()->fboPtr, outlineAlphaFactor);
  }

  // Begin drawing to main layer
  fboPtr0->getSource().begin();
  ofPushStyle();
  ofScale(fboPtr0->getWidth(), fboPtr0->getHeight());

  // Close the path for drawing the fill
  path.close();

  // Compute tint color based on strategy
  ofFloatColor tintColor;
  if (strategyParameter != 2) { // 2 == draw an untinted snapshot
    tintColor = colorController.value; // comes from a connected palette or manually
    float currentSaturation = tintColor.getSaturation();
    tintColor.setSaturation(std::clamp(currentSaturation * saturationController.value, 0.0f, 1.0f));
  } else {
    tintColor = ofFloatColor(1.0, 1.0, 1.0, 1.0);
  }

  ofEnableBlendMode(OF_BLENDMODE_SCREEN);

  // Execute the appropriate drawing strategy
  if (strategyParameter == 0) {
    drawStrategyTintFill(tintColor);
  } else {
    drawStrategySnapshot(tintColor);
  }

  ofPopStyle();
  fboPtr0->getSource().end();
  path.clear();
  snapshotTexture = ofTexture{}; // Reset to unallocated state so we wait for fresh texture
}

void CollageMod::receive(int sinkId, const ofTexture& texture) {
  switch (sinkId) {
    case SINK_SNAPSHOT_TEXTURE:
      snapshotTexture = texture;
      break;
    default:
      ofLogError("CollageMod") << "ofTexture receive for unknown sinkId " << sinkId;
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
  IntentMap im(intent);

  ofFloatColor energetic = energyToColor(intent);
  ofFloatColor structured = structureToBrightness(intent);
  ofFloatColor mixed = energetic.getLerped(structured, 0.25f);
  ofFloatColor finalColor = densityToAlpha(intent, mixed);
  colorController.updateIntent(finalColor, strength, "E->color, S->bright, D->alpha");

  float satEnergy = linearMap(im.E().get(), 0.8f, 2.2f);
  float satChaos = exponentialMap(im.C().get(), 0.9f, 2.8f, 2.0f);
  float satStructure = inverseMap(im.S().get(), 0.8f, 1.6f);
  float targetSaturation = std::clamp(satEnergy * satChaos * satStructure, 0.0f, 3.0f);
  saturationController.updateIntent(targetSaturation, strength, "E*C*inv(S)->sat");

  // OutlineAlphaFactor: High structure + low chaos = visible outlines
  // S increases alpha, C decreases it
  float outlineAlpha = linearMap(im.S().get(), 0.2f, 1.0f) * inverseMap(im.C().get(), 0.5f, 1.0f);
  outlineAlphaFactorController.updateIntent(std::clamp(outlineAlpha, 0.0f, 1.0f), strength, "S*inv(C)->alpha");

  // OutlineWidth: Energy increases boldness, granularity refines
  // High E = bold outlines, high G = finer detail (thinner)
  float widthEnergy = linearMap(im.E().get(), 8.0f, 24.0f);
  float widthGranularity = inverseMap(im.G().get(), 0.5f, 1.2f);
  float targetWidth = std::clamp(widthEnergy * widthGranularity, 1.0f, 50.0f);
  outlineWidthController.updateIntent(targetWidth, strength, "E*inv(G)->width");

  // OutlineColour: Contrast with fill - structure controls brightness, energy controls warmth
  ofFloatColor outlineColor;
  float brightness = inverseExponentialMap(im.S().get(), 0.3f, 1.0f); // high S = darker outlines for contrast
  float warmth = linearMap(im.E().get(), 0.0f, 0.15f); // subtle warm shift with energy
  outlineColor.r = std::clamp(brightness + warmth, 0.0f, 1.0f);
  outlineColor.g = std::clamp(brightness, 0.0f, 1.0f);
  outlineColor.b = std::clamp(brightness - warmth * 0.5f, 0.0f, 1.0f);
  outlineColor.a = 1.0f; // alpha handled by OutlineAlphaFactor
  outlineColorController.updateIntent(outlineColor, strength, "inv(S)->bright, E->warmth");
}



} // ofxMarkSynth
