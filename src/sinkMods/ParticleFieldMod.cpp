//
//  ParticleFieldMod.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 10/09/2025.
//

#include "ParticleFieldMod.hpp"


namespace ofxMarkSynth {


ParticleFieldMod::ParticleFieldMod(const std::string& name, const ModConfig&& config, float field1ValueOffset_, float field2ValueOffset_, int particleCount_)
: Mod { name, std::move(config) }
{
  particleField.setup(particleCount_, ofFloatColor(1.0, 1.0, 1.0, 0.3), field1ValueOffset_, field2ValueOffset_);
}

void ParticleFieldMod::initParameters() {
  parameters.add(particleField.getParameterGroup());
}

void ParticleFieldMod::update() {
  auto drawingLayerPtrOpt = getNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) return;
  auto fboPtr = drawingLayerPtrOpt.value()->fboPtr;
  
  particleField.update();
  
  // TODO: When the FboPtr config clears on update, particles ADD else ALPHA and pass in a reduced alpha multipler to the drawshader
  fboPtr->getSource().begin();
  ofClear(0, 0);
  ofEnableBlendMode(OF_BLENDMODE_ADD);
  fboPtr->getSource().end();
//  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  particleField.draw(fboPtr->getSource());
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


} // ofxMarkSynth
