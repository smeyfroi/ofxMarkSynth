//
//  SoftCircleMod.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 25/05/2025.
//

#include "SoftCircleMod.hpp"
#include "core/IntentMapping.hpp"
#include "core/IntentMapper.hpp"



namespace ofxMarkSynth {



SoftCircleMod::SoftCircleMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config)
: Mod { synthPtr, name, std::move(config) }
{
  softCircleShader.load();

  sinkNameIdMap = {
    { "Point", SINK_POINTS },
    { radiusParameter.getName(), SINK_RADIUS },
    { colorParameter.getName(), SINK_COLOR },
    { "ChangeKeyColour", SINK_CHANGE_KEY_COLOUR },
    { colorMultiplierParameter.getName(), SINK_COLOR_MULTIPLIER },
    { alphaMultiplierParameter.getName(), SINK_ALPHA_MULTIPLIER },
    { softnessParameter.getName(), SINK_SOFTNESS },
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
}

float SoftCircleMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

void SoftCircleMod::update() {
  syncControllerAgencies();
  auto drawingLayerPtrOpt = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) {
    newPoints.clear(); // Clear to prevent accumulation when paused
    return;
  }
  auto fboPtr = drawingLayerPtrOpt.value()->fboPtr;

  radiusController.update();
  float radius = radiusController.value;

  colorController.update();
  ofFloatColor color = colorController.value;

  colorMultiplierController.update();
  float multiplier = colorMultiplierController.value;
  color *= multiplier;

  alphaMultiplierController.update();
  float alphaMultiplier = alphaMultiplierController.value;
  color.a *= alphaMultiplier;

  softnessController.update();
  float softness = softnessController.value;

  int falloff = falloffParameter.get();

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

  std::for_each(newPoints.begin(),
                newPoints.end(),
                [&](const auto& p) {
    softCircleShader.render(p * fboPtr->getSize(), radius * fboPtr->getWidth(), color, softness, falloff);
  });

  ofPopStyle();
  fboPtr->getSource().end();

  newPoints.clear();
}

void SoftCircleMod::receive(int sinkId, const float& value) {
  if (sinkId != SINK_CHANGE_KEY_COLOUR && !canDrawOnNamedLayer()) return;

  switch (sinkId) {
    case SINK_CHANGE_KEY_COLOUR:
      if (value > 0.5f) {        keyColourRegister.ensureInitialized(keyColourRegisterInitialized, keyColoursParameter.get(), colorParameter.get());
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
  if (sinkId != SINK_CHANGE_KEY_COLOUR && !canDrawOnNamedLayer()) return;

  switch (sinkId) {
    case SINK_POINTS:
      newPoints.push_back(point);
      break;
    default:
      ofLogError("SoftCircleMod") << "glm::vec2 receive for unknown sinkId " << sinkId;
  }
}

void SoftCircleMod::receive(int sinkId, const glm::vec4& v) {
  if (sinkId != SINK_CHANGE_KEY_COLOUR && !canDrawOnNamedLayer()) return;

  switch (sinkId) {
    case SINK_COLOR:
      colorController.updateAuto(ofFloatColor { v.r, v.g, v.b, v.a }, getAgency());
      break;
    default:
      ofLogError("SoftCircleMod") << "glm::vec4 receive for unknown sinkId " << sinkId;
  }
}

void SoftCircleMod::applyIntent(const Intent& intent, float strength) {
  IntentMap im(intent);
  im.E().exp(radiusController, strength);
  im.D().exp(colorMultiplierController, strength, 1.0f, 1.4f, 4.0f); // D=0->1.0, D=0.8->1.16, D=1.0->1.4
  im.D().lin(alphaMultiplierController, strength);
}



} // ofxMarkSynth
