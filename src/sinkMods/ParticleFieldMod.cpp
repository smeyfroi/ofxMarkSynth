//
//  ParticleFieldMod.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 10/09/2025.
//

#include "ParticleFieldMod.hpp"
#include "cmath"


namespace ofxMarkSynth {


ParticleFieldMod::ParticleFieldMod(const std::string& name, const ModConfig&& config, float field1ValueOffset_, float field2ValueOffset_)
: Mod { name, std::move(config) }
{
  particleField.setup(ofFloatColor(1.0, 1.0, 1.0, 0.3), field1ValueOffset_, field2ValueOffset_);
}

void ParticleFieldMod::initParameters() {
  parameters.add(particleField.getParameterGroup());
}

void ParticleFieldMod::update() {
  auto drawingLayerPtrOpt = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) return;
  auto drawingLayerPtr = drawingLayerPtrOpt.value();
  auto fboPtr = drawingLayerPtr->fboPtr;
  
  particleField.update();
  
//  // Use ALPHA for layers that clear on update, else SCREEN
//  if (drawingLayerPtr->clearOnUpdate) ofEnableBlendMode(OF_BLENDMODE_SCREEN);
//  else ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  ofEnableBlendMode(OF_BLENDMODE_SCREEN);
  
  particleField.draw(fboPtr->getSource(), drawingLayerPtr->fadeBy < 0.8f);
}

void ParticleFieldMod::receive(int sinkId, const ofFbo& value) {
  switch (sinkId) {
    case SINK_FIELD_1_FBO:
      particleField.setField1(value.getTexture());
      break;
    case SINK_FIELD_2_FBO:
      particleField.setField2(value.getTexture());
      break;
    default:
      ofLogError() << "ofFbo receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
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
      ofLogError() << "glm::vec4 receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}

void ParticleFieldMod::receive(int sinkId, const float& value) {
  switch (sinkId) {
    case SINK_CHANGE_LAYER:
      if (value > 0.8) { // FIXME: temp until connections have weights
        ofLogNotice() << "ParticleFieldMod::SINK_CHANGE_LAYER: changing layer";
        changeDrawingLayer();
      } else if (value > 0.2) {
        ofLogNotice() << "ParticleFieldMod::SINK_CHANGE_LAYER: resetting layer";
        resetDrawingLayer();
      }
      break;
    default:
      ofLogError() << "float receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}



} // ofxMarkSynth
