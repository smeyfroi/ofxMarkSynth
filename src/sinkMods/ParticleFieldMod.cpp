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
  auto drawingLayerPtrOpt = getNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) return;
  auto drawingLayerPtr = drawingLayerPtrOpt.value();
  auto fboPtr = drawingLayerPtr->fboPtr;
  
  particleField.update();
  
  // Use ADD for layers that clear on update, else ALPHA
  if (drawingLayerPtr->clearOnUpdate) ofEnableBlendMode(OF_BLENDMODE_ADD);
  else ofEnableBlendMode(OF_BLENDMODE_ALPHA);

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
