//
//  SoftCircleMod.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 25/05/2025.
//

#include "SoftCircleMod.hpp"

#include <algorithm>
#include <cmath>

#include "core/IntentMapping.hpp"
#include "core/IntentMapper.hpp"

namespace ofxMarkSynth {

SoftCircleMod::SoftCircleMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config)
: Mod { synthPtr, name, std::move(config) }
{
  softCircleShader.load();

  sinkNameIdMap = {
    { "Point", SINK_POINTS },
    { "PointVelocity", SINK_POINT_VELOCITY },
    { radiusParameter.getName(), SINK_RADIUS },
    { colorParameter.getName(), SINK_COLOR },
    { "ChangeKeyColour", SINK_CHANGE_KEY_COLOUR },
    { colorMultiplierParameter.getName(), SINK_COLOR_MULTIPLIER },
    { alphaMultiplierParameter.getName(), SINK_ALPHA_MULTIPLIER },
    { softnessParameter.getName(), SINK_SOFTNESS },
    { "EdgeMod", SINK_EDGE_MOD },
    { "ChangeLayer", Mod::SINK_CHANGE_LAYER }
  };

  registerControllerForSource(radiusParameter, radiusController);
  registerControllerForSource(colorParameter, colorController);
  registerControllerForSource(colorMultiplierParameter, colorMultiplierController);
  registerControllerForSource(alphaMultiplierParameter, alphaMultiplierController);
  registerControllerForSource(softnessParameter, softnessController);
}

void SoftCircleMod::initParameters() {
  parameters.add(radiusParameter);
  parameters.add(colorParameter);
  parameters.add(keyColoursParameter);
  parameters.add(colorMultiplierParameter);
  parameters.add(alphaMultiplierParameter);
  parameters.add(softnessParameter);
  parameters.add(falloffParameter);
  parameters.add(agencyFactorParameter);

  parameters.add(useVelocityParameter);
  parameters.add(velocitySpeedMinParameter);
  parameters.add(velocitySpeedMaxParameter);
  parameters.add(velocityStretchParameter);
  parameters.add(velocityAngleInfluenceParameter);
  parameters.add(preserveAreaParameter);
  parameters.add(maxAspectParameter);

  parameters.add(curvatureAlphaBoostParameter);
  parameters.add(curvatureRadiusBoostParameter);
  parameters.add(curvatureSmoothingParameter);

  parameters.add(edgeAmountParameter);
  parameters.add(edgeAmountFromDriverParameter);
  parameters.add(edgeSharpnessParameter);
  parameters.add(edgeFreq1Parameter);
  parameters.add(edgeFreq2Parameter);
  parameters.add(edgeFreq3Parameter);
  parameters.add(edgePhaseFromVelocityParameter);
}

float SoftCircleMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

void SoftCircleMod::update() {
  syncControllerAgencies();
  auto drawingLayerPtrOpt = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) {
    newStamps.clear(); // Clear to prevent accumulation when paused
    return;
  }
  auto fboPtr = drawingLayerPtrOpt.value()->fboPtr;

  radiusController.update();
  float radius = radiusController.value;

  colorController.update();
  ofFloatColor baseColor = colorController.value;

  colorMultiplierController.update();
  float multiplier = colorMultiplierController.value;
  baseColor *= multiplier;

  alphaMultiplierController.update();
  float alphaMultiplier = alphaMultiplierController.value;
  baseColor.a *= alphaMultiplier;

  softnessController.update();
  float softness = softnessController.value;

  int falloff = falloffParameter.get();

  const bool useVelocity = useVelocityParameter.get() > 0;
  const float speedMin = velocitySpeedMinParameter.get();
  const float speedMax = velocitySpeedMaxParameter.get();
  const float stretch = velocityStretchParameter.get();
  const float angleInfluence = velocityAngleInfluenceParameter.get();
  const bool preserveArea = preserveAreaParameter.get() > 0;
  const float maxAspect = maxAspectParameter.get();

  const float curvatureAlphaBoost = curvatureAlphaBoostParameter.get();
  const float curvatureRadiusBoost = curvatureRadiusBoostParameter.get();

  const float edgeBase = edgeAmountParameter.get();
  const float edgeFromDriver = edgeAmountFromDriverParameter.get();
  const glm::vec3 edgeFreq { edgeFreq1Parameter.get(), edgeFreq2Parameter.get(), edgeFreq3Parameter.get() };
  const float edgeSharpness = edgeSharpnessParameter.get();
  const bool edgePhaseFromVelocity = edgePhaseFromVelocityParameter.get() > 0;

  fboPtr->getSource().begin();

  ofPushStyle();
  if (falloff == 1) {
    // Dab: premultiplied alpha blend for proper compositing without halos
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  } else {
    // Glow: standard alpha blending
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  }

  const float baseRadiusPx = radius * fboPtr->getWidth();

  std::for_each(newStamps.begin(), newStamps.end(), [&](const auto& stamp) {
    const glm::vec2 centerPx = stamp.pointNorm * fboPtr->getSize();

    ofFloatColor stampColor = baseColor;
    stampColor.a *= (1.0f + curvatureAlphaBoost * stamp.curvature);
    stampColor.a = std::clamp(stampColor.a, 0.0f, 1.0f);

    float radiusPx = baseRadiusPx * (1.0f + curvatureRadiusBoost * stamp.curvature);

    float majorR = radiusPx;
    float minorR = radiusPx;
    float angleRad = 0.0f;
    glm::vec2 dirUnit { 1.0f, 0.0f };

    if (useVelocity && stamp.hasVelocity && stretch > 0.0f) {
      float speed = glm::length(stamp.velocityNorm);
      if (speed > 1e-6f) {
        dirUnit = stamp.velocityNorm / speed;

        float speedNorm = 0.0f;
        if (speedMax > speedMin + 1e-6f) {
          speedNorm = std::clamp((speed - speedMin) / (speedMax - speedMin), 0.0f, 1.0f);
        }

        float aspect = 1.0f + speedNorm * stretch;
        aspect = std::clamp(aspect, 1.0f, maxAspect);

        if (preserveArea) {
          float s = std::sqrt(aspect);
          majorR = radiusPx * s;
          minorR = radiusPx / s;
        } else {
          majorR = radiusPx * aspect;
          minorR = radiusPx;
        }

        if (angleInfluence > 0.0f) {
          glm::vec2 blendedDir = glm::normalize(glm::mix(glm::vec2 { 1.0f, 0.0f }, dirUnit, angleInfluence));
          angleRad = atan2(blendedDir.y, blendedDir.x);
        }
      }
    }

    glm::vec2 sizePx { majorR * 2.0f, minorR * 2.0f };

    float edgeAmount = edgeBase + edgeFromDriver * stamp.edgeDriver;
    edgeAmount = std::clamp(edgeAmount, 0.0f, 1.0f);

    float edgePhase = 0.0f;
    if (edgePhaseFromVelocity && stamp.hasVelocity) {
      edgePhase = atan2(dirUnit.y, dirUnit.x);
    }

    softCircleShader.render(centerPx,
                            sizePx,
                            angleRad,
                            stampColor,
                            softness,
                            falloff,
                            edgeAmount,
                            edgeFreq,
                            edgeSharpness,
                            edgePhase);
  });

  ofPopStyle();
  fboPtr->getSource().end();

  newStamps.clear();
}

void SoftCircleMod::receive(int sinkId, const float& value) {
  if (sinkId != SINK_CHANGE_KEY_COLOUR && sinkId != SINK_EDGE_MOD && !canDrawOnNamedLayer()) return;

  switch (sinkId) {
    case SINK_CHANGE_KEY_COLOUR:
      if (value > 0.5f) {
        keyColourRegister.ensureInitialized(keyColourRegisterInitialized, keyColoursParameter.get(), colorParameter.get());
        keyColourRegister.flip();
        colorParameter.set(keyColourRegister.getCurrentColour());
      }
      break;

    case SINK_RADIUS:
      radiusController.updateAuto(value, getAgency());
      break;
    case SINK_COLOR_MULTIPLIER:
      colorMultiplierController.updateAuto(value, getAgency());
      break;
    case SINK_ALPHA_MULTIPLIER:
      alphaMultiplierController.updateAuto(value, getAgency());
      break;
    case SINK_SOFTNESS:
      softnessController.updateAuto(value, getAgency());
      break;

    case SINK_EDGE_MOD:
      currentEdgeDriver = std::clamp(value, 0.0f, 1.0f);
      break;

    case Mod::SINK_CHANGE_LAYER:
      if (value > 0.5f) {
        ofLogNotice("SoftCircleMod") << "SoftCircleMod::ChangeLayer: changing layer";
        changeDrawingLayer();
      }
      break;
    default:
      ofLogError("SoftCircleMod") << "Float receive for unknown sinkId " << sinkId;
  }
}

void SoftCircleMod::receive(int sinkId, const glm::vec2& point) {
  if (!canDrawOnNamedLayer()) return;

  switch (sinkId) {
    case SINK_POINTS: {
      Stamp stamp;
      stamp.pointNorm = point;
      stamp.edgeDriver = currentEdgeDriver;

      if (lastPointOpt) {
        glm::vec2 vel = point - lastPointOpt.value();
        float speed = glm::length(vel);
        if (speed > 1e-6f) {
          stamp.hasVelocity = true;
          stamp.velocityNorm = vel;

          glm::vec2 dirUnit = vel / speed;
          if (lastDirectionOpt) {
            float curvRaw = 1.0f - glm::dot(lastDirectionOpt.value(), dirUnit);
            curvRaw = std::clamp(curvRaw * 0.5f, 0.0f, 1.0f);
            float smoothing = curvatureSmoothingParameter.get();
            lastCurvature = smoothing * lastCurvature + (1.0f - smoothing) * curvRaw;
          } else {
            lastCurvature = 0.0f;
          }
          lastDirectionOpt = dirUnit;
          stamp.curvature = lastCurvature;
        }
      }

      lastPointOpt = point;
      newStamps.push_back(stamp);
      break;
    }
    default:
      ofLogError("SoftCircleMod") << "glm::vec2 receive for unknown sinkId " << sinkId;
  }
}

void SoftCircleMod::receive(int sinkId, const glm::vec4& v) {
  if (sinkId != SINK_CHANGE_KEY_COLOUR && !canDrawOnNamedLayer()) return;

  switch (sinkId) {
    case SINK_POINT_VELOCITY: {
      Stamp stamp;
      stamp.pointNorm = { v.x, v.y };
      stamp.velocityNorm = { v.z, v.w };
      stamp.edgeDriver = currentEdgeDriver;

      float speed = glm::length(stamp.velocityNorm);
      if (speed > 1e-6f) {
        stamp.hasVelocity = true;
        glm::vec2 dirUnit = stamp.velocityNorm / speed;
        if (lastDirectionOpt) {
          float curvRaw = 1.0f - glm::dot(lastDirectionOpt.value(), dirUnit);
          curvRaw = std::clamp(curvRaw * 0.5f, 0.0f, 1.0f);
          float smoothing = curvatureSmoothingParameter.get();
          lastCurvature = smoothing * lastCurvature + (1.0f - smoothing) * curvRaw;
        } else {
          lastCurvature = 0.0f;
        }
        lastDirectionOpt = dirUnit;
        stamp.curvature = lastCurvature;
      }

      lastPointOpt = stamp.pointNorm;
      newStamps.push_back(stamp);
      break;
    }

    case SINK_COLOR:
      colorController.updateAuto(ofFloatColor { v.r, v.g, v.b, v.a }, getAgency());
      break;
    default:
      ofLogError("SoftCircleMod") << "glm::vec4 receive for unknown sinkId " << sinkId;
  }
}

void SoftCircleMod::applyIntent(const Intent& intent, float strength) {
  IntentMap im(intent);

  // Keep intent changes bounded around the tuned baseline.
  im.E().expAround(radiusController, strength);
  im.D().expAround(colorMultiplierController, strength);

  // Alpha is very sensitive (blowouts + persistent layers), so modulate it gently.
  im.D().expAround(alphaMultiplierController, strength);
}

} // namespace ofxMarkSynth
