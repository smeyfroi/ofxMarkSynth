//
//  ParticleFieldMod.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 10/09/2025.
//

#include "ParticleFieldMod.hpp"
#include "cmath"
#include "IntentMapping.hpp"
#include "Parameter.hpp"
#include "../IntentMapper.hpp"


namespace ofxMarkSynth {


ParticleFieldMod::ParticleFieldMod(Synth* synthPtr, const std::string& name, ModConfig config, float field1ValueOffset_, float field2ValueOffset_)
: Mod { synthPtr, name, std::move(config) }
{
  particleField.setup(ofFloatColor(1.0, 1.0, 1.0, 0.3), field1ValueOffset_, field2ValueOffset_);
  
  sinkNameIdMap = {
    { "Field1Texture", SINK_FIELD_1_FBO },
    { "Field2Texture", SINK_FIELD_2_FBO },
    { "ColourFieldTexture", SINK_COLOR_FIELD_FBO },
    { "PointColour", SINK_POINT_COLOR },
    { "MinWeight", SINK_MIN_WEIGHT },
    { "MaxWeight", SINK_MAX_WEIGHT }
  };
}

void ParticleFieldMod::initParameters() {
  addFlattenedParameterGroup(parameters, particleField.getParameterGroup());
  parameters.add(agencyFactorParameter);
  
  minWeightControllerPtr = std::make_unique<ParamController<float>>(parameters.get("minWeight").cast<float>());
  maxWeightControllerPtr = std::make_unique<ParamController<float>>(parameters.get("maxWeight").cast<float>());
  
  sourceNameControllerPtrMap = {
    { parameters.get("minWeight").cast<float>().getName(), minWeightControllerPtr.get() },
    { parameters.get("maxWeight").cast<float>().getName(), maxWeightControllerPtr.get() }
  };
}

float ParticleFieldMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

void ParticleFieldMod::update() {
  syncControllerAgencies();
  minWeightControllerPtr->update();
  maxWeightControllerPtr->update();
  
  auto drawingLayerPtrOpt = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) return;
  auto drawingLayerPtr = drawingLayerPtrOpt.value();
  auto fboPtr = drawingLayerPtr->fboPtr;
  
  particleField.update();
  
//  // Use ALPHA for layers that clear on update, else SCREEN
//  if (drawingLayerPtr->clearOnUpdate) ofEnableBlendMode(OF_BLENDMODE_SCREEN);
//  else ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  ofEnableBlendMode(OF_BLENDMODE_SCREEN);
  
  particleField.draw(fboPtr->getSource(), !drawingLayerPtr->clearOnUpdate);
}

void ParticleFieldMod::receive(int sinkId, const ofTexture& value) {
  switch (sinkId) {
    case SINK_FIELD_1_FBO:
      particleField.setField1(value);
      break;
    case SINK_FIELD_2_FBO:
      particleField.setField2(value);
      break;
    default:
      ofLogError("ParticleFieldMod") << "ofFbo receive for unknown sinkId " << sinkId;
  }
}

void ParticleFieldMod::receive(int sinkId, const glm::vec4& v) {
  switch (sinkId) {
    case SINK_POINT_COLOR: {
      const auto color = ofFloatColor { v.r, v.g, v.b, v.a };
      auto particleBlocks = particleField.getParticleCount() / 64;
      auto updateBlocks = std::max(1, std::min(particleBlocks / 100, 64)); // update 1% of particles up to 64 blocks, min 1 block
      particleField.updateRandomColorBlocks(updateBlocks, 64, [&color](size_t idx) {
        return color;
      });
      break;
    }
    default:
      ofLogError("ParticleFieldMod") << "glm::vec4 receive for unknown sinkId " << sinkId;
  }
}

void ParticleFieldMod::receive(int sinkId, const float& value) {
  switch (sinkId) {
    case SINK_CHANGE_LAYER:
      if (value > 0.8) { // FIXME: temp until connections have weights
        ofLogNotice("ParticleFieldMod") << "ParticleFieldMod::SINK_CHANGE_LAYER: changing layer";
        changeDrawingLayer();
      } else if (value > 0.2) {
        ofLogNotice("ParticleFieldMod") << "ParticleFieldMod::SINK_CHANGE_LAYER: resetting layer";
        resetDrawingLayer();
      }
      break;
    case SINK_MIN_WEIGHT:
      minWeightControllerPtr->updateAuto(value, getAgency());
      break;
    case SINK_MAX_WEIGHT:
      maxWeightControllerPtr->updateAuto(value, getAgency());
      break;
    default:
      ofLogError("ParticleFieldMod") << "Float receive for unknown sinkId " << sinkId;
  }
}

void ParticleFieldMod::applyIntent(const Intent& intent, float strength) {
  IntentMap im(intent);
  
  // Color composition
  ofFloatColor color = ofxMarkSynth::energyToColor(intent);
  color.setBrightness(ofxMarkSynth::structureToBrightness(intent) * 0.5f);
  color.setSaturation(im.E().get() * im.C().get());
  color.a = ofLerp(0.1f, 0.5f, im.D().get());
  auto particleBlocks = particleField.getParticleCount() / 64;
  auto updateBlocks = std::max(1, static_cast<int>(particleBlocks * std::clamp(strength * im.D().get(), 0.01f, 1.0f)));
  particleField.updateRandomColorBlocks(updateBlocks, 64, [&color](size_t idx) {
    return color;
  });
  
  im.G().inv().lin(*minWeightControllerPtr, strength);
  im.C().lin(*maxWeightControllerPtr, strength);
}



} // ofxMarkSynth
