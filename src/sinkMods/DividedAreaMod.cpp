//
//  DividedAreaMod.cpp
//  example_audio_clusters
//
//  Created by Steve Meyfroidt on 07/05/2025.
//

#include "DividedAreaMod.hpp"
#include "LineGeom.h"
#include "IntentMapping.hpp"
#include "Parameter.hpp"
#include "../IntentMapper.hpp"


namespace ofxMarkSynth {


DividedAreaMod::DividedAreaMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config)
: Mod { synthPtr, name, std::move(config) },
dividedArea({ { 1.0, 1.0 }, static_cast<int>(maxUnconstrainedLinesParameter.get()) }) // normalised area size
{
  sinkNameIdMap = {
    { "MajorAnchor", SINK_MAJOR_ANCHORS },
    { "MinorAnchor", SINK_MINOR_ANCHORS },
    { "MinorPath", SINK_MINOR_PATH },
    { minorLineColorParameter.getName(), SINK_MINOR_LINES_COLOR },
    { majorLineColorParameter.getName(), SINK_MAJOR_LINES_COLOR },
    { "BackgroundFbo", SINK_BACKGROUND_SOURCE },
    { "ChangeAngle", SINK_CHANGE_ANGLE },
    { "ChangeStrategy", SINK_CHANGE_STRATEGY },
    { "ChangeLayer", SINK_CHANGE_LAYER }
  };
  
  registerControllerForSource(angleParameter, angleController);
  registerControllerForSource(minorLineColorParameter, minorLineColorController);
  registerControllerForSource(majorLineColorParameter, majorLineColorController);
  registerControllerForSource(pathWidthParameter, pathWidthController);
  registerControllerForSource(majorLineWidthParameter, majorLineWidthController);
  registerControllerForSource(maxUnconstrainedLinesParameter, maxUnconstrainedLinesController);
}

void DividedAreaMod::initParameters() {
  parameters.add(strategyParameter);
  parameters.add(angleParameter);
  parameters.add(pathWidthParameter);
  parameters.add(majorLineWidthParameter);
  parameters.add(minorLineColorParameter);
  parameters.add(majorLineColorParameter);
  parameters.add(maxUnconstrainedLinesParameter);
  parameters.add(agencyFactorParameter);
  addFlattenedParameterGroup(parameters, dividedArea.getParameterGroup());
}

float DividedAreaMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

void DividedAreaMod::addConstrainedLinesThroughPointPairs(float width) {
  const ofFloatColor minorDividerColor = minorLineColorController.value;
  int pairs = newMinorAnchors.size() / 2;
  for (int i = 0; i < pairs; i++) {
    auto p1 = newMinorAnchors.back();
    newMinorAnchors.pop_back();
    auto p2 = newMinorAnchors.back();
    newMinorAnchors.pop_back();
    dividedArea.addConstrainedDividerLine(p1, p2, minorDividerColor, width);
  }
}

void DividedAreaMod::addConstrainedLinesThroughPointAngles() {
  const ofFloatColor minorDividerColor = minorLineColorController.value;
  float angle = angleController.value;
  std::for_each(newMinorAnchors.begin(), newMinorAnchors.end(), [&](const auto& p) {
    auto endPoint = endPointForSegment(p, angleController.value * glm::pi<float>(), 0.01);
    if (endPoint.x > 0.0 && endPoint.x < 1.0 && endPoint.y > 0.0 && endPoint.y < 1.0) {
      // must stay inside normalised coords
      dividedArea.addConstrainedDividerLine(p, endPoint, minorDividerColor);
    }
  });
  newMinorAnchors.clear();
}

void DividedAreaMod::addConstrainedLinesRadiating() {
  if (newMinorAnchors.size() < 7) return;
  const ofFloatColor minorDividerColor = minorLineColorController.value;
  glm::vec2 centrePoint = newMinorAnchors.back(); newMinorAnchors.pop_back();
  std::for_each(newMinorAnchors.begin(), newMinorAnchors.end(), [&](const auto& p) {
    dividedArea.addConstrainedDividerLine(centrePoint, p, minorDividerColor);
  });
  newMinorAnchors.clear();
}

void DividedAreaMod::update() {
  syncControllerAgencies();
  angleController.update();
  minorLineColorController.update();
  majorLineColorController.update();
  pathWidthController.update();
  majorLineWidthController.update();
  maxUnconstrainedLinesController.update();
  dividedArea.maxUnconstrainedDividerLines = static_cast<int>(maxUnconstrainedLinesController.value);
  
  dividedArea.updateUnconstrainedDividerLines(newMajorAnchors); // assumes all the major anchors come at once (as the cluster centres)
  newMajorAnchors.clear();
  
  if (!newMinorAnchors.empty()) {
    switch (strategyParameter) {
      case 0:
        addConstrainedLinesThroughPointPairs();
        break;
      case 1:
        addConstrainedLinesThroughPointAngles();
        break;
      case 2:
        addConstrainedLinesRadiating();
        break;
      default:
        break;
    }
  }
  
//  const float maxLineWidth = 160.0;
//  const float minLineWidth = 110.0;

  // draw unconstrained
  auto drawingLayerPtrOpt0 = getCurrentNamedDrawingLayerPtr(MAJOR_LINES_LAYERPTR_NAME);
  if (drawingLayerPtrOpt0) {
    auto fboPtr0 = drawingLayerPtrOpt0.value()->fboPtr;
    
    // need a background for the refraction effect
    if (backgroundFbo.isAllocated()) {
      fboPtr0->getSource().begin();
      const ofFloatColor majorDividerColor = majorLineColorController.value;
      // TODO: bring back the flat coloured major lines?
      //    dividedArea.draw({},
      //                     { minLineWidth, maxLineWidth, majorDividerColor },
      //                     fboPtr0->getWidth());
      ofSetColor(majorDividerColor);
      dividedArea.draw(0.0, majorLineWidthController.value, fboPtr0->getWidth(), backgroundFbo);
      fboPtr0->getSource().end();
    }
  }
  
  // draw constrained
  auto drawingLayerPtrOpt1 = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt1) return;
  auto fboPtr1 = drawingLayerPtrOpt1.value()->fboPtr;
  fboPtr1->getSource().begin();
  dividedArea.drawInstanced(fboPtr1->getWidth());
//    ofSetColor(minorDividerColor);
//    dividedArea.draw(0.0, 0.0, 10.0, fboPtr1->getWidth());
  fboPtr1->getSource().end();
}

void DividedAreaMod::receive(int sinkId, const glm::vec2& point) {
  switch (sinkId) {
    case SINK_MAJOR_ANCHORS:
      if (newMajorAnchors.empty() || newMajorAnchors.back() != point) newMajorAnchors.push_back(point);
      break;
    case SINK_MINOR_ANCHORS:
      if (newMinorAnchors.empty() || newMinorAnchors.back() != point) newMinorAnchors.push_back(point);
      break;
    default:
      ofLogError("DividedAreaMod") << "glm::vec2 receive for unknown sinkId " << sinkId;
  }
}

void DividedAreaMod::receive(int sinkId, const ofPath& path) {
  switch (sinkId) {
    case SINK_MINOR_PATH:
      // fetch all the points of the path
      for (auto& poly : path.getOutline()) {
        auto vertices = poly.getVertices();
        glm::vec2 previousVertex { vertices.back() };
        for (auto& v : vertices) {
          glm::vec2 p { v };
          if (newMinorAnchors.empty() || newMinorAnchors.back() != p) {
            newMinorAnchors.push_back(previousVertex);
            newMinorAnchors.push_back(p);
            previousVertex = p;
          }
        }
      }
      addConstrainedLinesThroughPointPairs(pathWidthController.value); // add lines for a path immediately
      break;
    default:
      ofLogError("DividedAreaMod") << "ofPath receive for unknown sinkId " << sinkId;
  }
}

void DividedAreaMod::receive(int sinkId, const float& v) {
  switch (sinkId) {
    case SINK_CHANGE_LAYER:
      if (v > 0.6) { // FIXME: temp until connections have weights
        ofLogNotice("DividedAreaMod") << "DividedAreaMod::SINK_CHANGE_LAYER: changing layer";
        changeDrawingLayer();
      }
      break;
    case SINK_CHANGE_ANGLE:
      {
        if (v > 0.4) { // FIXME: temp until connections have weights
          float newAngle = v;
          ofLogNotice("DividedAreaMod") << "DividedAreaMod::SINK_CHANGE_ANGLE: changing angle to " << newAngle;
          angleController.updateAuto(newAngle, getAgency());
          angleParameter = newAngle;
        }
      }
      break;
    case SINK_CHANGE_STRATEGY:
    {
      if (ofGetElapsedTimef() < strategyChangeInvalidUntilTimestamp) break;
      int newStrategy = (strategyParameter + 1) % 3;
      ofLogNotice("DividedAreaMod") << "DividedAreaMod::SINK_CHANGE_STRATEGY: changing strategy to " << newStrategy;
      strategyParameter = newStrategy;
      strategyChangeInvalidUntilTimestamp = ofGetElapsedTimef() + 5.0; // 5s. FIXME: should be a parameter
    }
      break;
    default:
      ofLogError("DividedAreaMod") << "Float receive for unknown sinkId " << sinkId;
  }
}

void DividedAreaMod::receive(int sinkId, const glm::vec4& v) {
  switch (sinkId) {
    case SINK_MINOR_LINES_COLOR:
      minorLineColorController.updateAuto(ofFloatColor { v.x, v.y, v.z, v.w }, getAgency());
      break;
    case SINK_MAJOR_LINES_COLOR:
      majorLineColorController.updateAuto(ofFloatColor { v.x, v.y, v.z, v.w }, getAgency());
      break;
    default:
      ofLogError("DividedAreaMod") << "glm::vec4 receive for unknown sinkId " << sinkId;
  }
}

void DividedAreaMod::receive(int sinkId, const ofFbo& v) {
  switch (sinkId) {
    case SINK_BACKGROUND_SOURCE:
      backgroundFbo = v;
      break;
    default:
      ofLogError("DividedAreaMod") << "ofFbo receive for unknown sinkId " << sinkId;
  }
}

void DividedAreaMod::applyIntent(const Intent& intent, float strength) {
  IntentMap im(intent);
  
  im.C().exp(angleController, strength, 0.0f, 0.5f, 2.0f);
  im.G().exp(pathWidthController, strength, 0.7f);
  
  // Minor color composition
  ofFloatColor minorColor = ofxMarkSynth::energyToColor(intent);
  minorColor.a = ofxMarkSynth::linearMap(intent.getDensity(), 0.7f, 1.0f);
  minorLineColorController.updateIntent(minorColor, strength, "E->color, D->alpha");
  
  // Major color composition
  ofFloatColor majorColor = ofxMarkSynth::energyToColor(intent) * 0.7f;
  majorColor.setBrightness(ofxMarkSynth::structureToBrightness(intent) * 0.8f);
  majorColor.setSaturation(intent.getEnergy() * intent.getStructure() * 0.5f);
  majorColor.a = ofxMarkSynth::exponentialMap(intent.getDensity(), 0.0f, 1.0f, 0.5f);
  majorLineColorController.updateIntent(majorColor, strength, "E->color, S->bright/sat, D->alpha");
  
  im.C().exp(maxUnconstrainedLinesController, strength, 1.0f, 9.0f, 2.0f);
  im.G().lin(majorLineWidthController, strength);
  
  // Strategy selection based on structure
  if (strength > 0.05f) {
    float s = intent.getStructure();
    int strategy = (s < 0.3f) ? 0 : (s < 0.7f ? 1 : 2);
    if (strategyParameter.get() != strategy) strategyParameter.set(strategy);
  }
}



} // ofxMarkSynth
