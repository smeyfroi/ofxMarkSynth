//
//  PathMod.cpp
//  example_collage
//
//  Created by Steve Meyfroidt on 19/05/2025.
//

#include "PathMod.hpp"
#include "ofxConvexHull.h"
#include "IntentMapping.hpp"
#include "IntentMapper.hpp"


namespace ofxMarkSynth {


PathMod::PathMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config)
: Mod { synthPtr, name, std::move(config) }
{
  sinkNameIdMap = {
    { "Point", SINK_VEC2 }
  };
  sourceNameIdMap = {
    { "Path", SOURCE_PATH }
  };
  
  registerControllerForSource(maxVerticesParameter, maxVerticesController);
  registerControllerForSource(clusterRadiusParameter, clusterRadiusController);
}

void PathMod::initParameters() {
  parameters.add(strategyParameter);
  parameters.add(maxVerticesParameter);
  parameters.add(clusterRadiusParameter);
  parameters.add(agencyFactorParameter);
}

float PathMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

std::vector<glm::vec2> PathMod::findCloseNewPoints() const {
  std::vector<glm::vec2> result;
  if (newVecs.empty()) return result;
  
  // Reference point is the newest point
  const glm::vec2& referencePoint = newVecs.back();
  float maxDistance = clusterRadiusController.value;
  
  // Collect all points within proximity of the reference point (cluster)
  for (const auto& v : newVecs) {
    float dist = glm::distance(referencePoint, v);
    if (dist <= maxDistance) {
      result.push_back(v);
    }
  }
  return result;
}

ofPath makePolyPath(const std::vector<glm::vec2>& points) {
  ofPath path;
  bool firstVertex = true;
  std::for_each(points.crbegin(), points.crend(), [&](const auto& p) {
    if (firstVertex) {
      path.moveTo(p);
      firstVertex = false;
    } else {
      path.lineTo(p);
    }
  });
  path.close();
  return path;
}

ofPath makeConvexHullPath(const std::vector<glm::vec2>& points) {
  std::vector<ofPoint> ofPoints(points.size());
  std::transform(points.cbegin(), points.cend(), std::back_inserter(ofPoints),
                 [](const glm::vec2& v) { return ofPoint { v.x, v.y }; });
  
  ofxConvexHull convexHull;
  vector<ofPoint>hullOfPoints = convexHull.getConvexHull(ofPoints);
  
  std::vector<glm::vec2> hullPoints;
  auto validEndIter = std::remove_if(hullOfPoints.begin(), hullOfPoints.end(),
                                     [](const auto& p) { return (std::abs(p.x) < std::numeric_limits<float>::epsilon() && std::abs(p.y) < std::numeric_limits<float>::epsilon()); });
  std::transform(hullOfPoints.begin(), validEndIter, std::back_inserter(hullPoints),
                 [](const ofPoint& v) { return glm::vec2 { v.x, v.y }; });
  
  ofPath result = makePolyPath(hullPoints);
  result.close();
  return result;
}

ofPath makeBoundsPath(const std::vector<glm::vec2>& points) {
  glm::vec2 tl { std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
  glm::vec2 br { std::numeric_limits<float>::min(), std::numeric_limits<float>::min() };
  std::for_each(points.cbegin(), points.cend(), [&](const auto& p) {
    if (p.x < tl.x) tl.x = p.x;
    if (p.x > br.x) br.x = p.x;
    if (p.y < tl.y) tl.y = p.y;
    if (p.y > br.y) br.y = p.y;
  });
  ofPath path;
  path.moveTo(tl); path.lineTo(br.x, tl.y); path.lineTo(br); path.lineTo(tl.x, br.y);
  path.close();
  return path;
}

ofPath makeHorizontalStripesPath(const std::vector<glm::vec2>& points) {
  ofPath path;
  for (auto iter = points.cbegin(); iter < points.cend() - 1; iter++) {
    auto p1 = *iter; iter++; auto p2 = *iter;
    path.moveTo(0.0, p1.y); path.lineTo(1.0, p1.y); path.lineTo(1.0, p2.y); path.lineTo(0.0, p2.y);
  }
  path.close();
  return path;
}

void PathMod::update() {
  syncControllerAgencies();
  maxVerticesController.update();
  clusterRadiusController.update();
  
  auto points = findCloseNewPoints();
  if (points.size() < 4) {
    if (newVecs.size() > maxVerticesController.value * 3.0) newVecs.pop_front(); // not finding anything so pop one to avoid growing too large
    return;
  }

  switch (strategyParameter) {
    case 0:
      path = makePolyPath(points);
      break;
    case 1:
      path = makeBoundsPath(points);
      break;
    case 2:
      path = makeHorizontalStripesPath(points);
      break;
    case 3:
      path = makeConvexHullPath(points);
      break;
    default:
      break;
  }

  path.setColor(ofFloatColor(1.0, 1.0, 1.0, 1.0));
//  path.setFilled(true);
  newVecs.clear();
  emit(SOURCE_PATH, path);
}

void PathMod::draw() {
  if (!visible) return;
  path.draw();
}

bool PathMod::keyPressed(int key) {
  if (key == 'A') {
    visible = not visible;
    return true;
  }
  return false;
}

void PathMod::applyIntent(const Intent& intent, float strength) {
  IntentMap im(intent);
  
  // Granularity -> larger cluster radius (bigger shapes)
  im.G().exp(clusterRadiusController, strength);
  // Density -> more vertices allowed
  im.D().exp(maxVerticesController, strength);
  
  // Strategy selection based on chaos + structure
//  if (strength > 0.05f) {
//    if (im.C().get() > 0.6f && im.S().get() < 0.4f) {
//      strategyParameter = 2; // horizontals - chaotic, unstructured
//    } else if (im.S().get() > 0.7f) {
//      strategyParameter = 3; // convex hull - highly structured
//    } else if (im.S().get() < 0.3f) {
//      strategyParameter = 0; // polypath - low structure
//    } else {
//      strategyParameter = 1; // bounds - middle ground
//    }
//  }
}

void PathMod::receive(int sinkId, const glm::vec2& v) {
  switch (sinkId) {
    case SINK_VEC2:
      if (!newVecs.empty() && newVecs.back() == v) return;
      newVecs.push_back(v);
      break;
    default:
      ofLogError("PathMod") << "glm::vec2 receive for unknown sinkId " << sinkId;
  }
}



} // ofxMarkSynth
