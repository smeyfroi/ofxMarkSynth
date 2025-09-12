//
//  ParticleFieldMod.cpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 10/09/2025.
//

#include "ParticleFieldMod.hpp"


namespace ofxMarkSynth {


ParticleFieldMod::ParticleFieldMod(const std::string& name, const ModConfig&& config, float fieldValueOffset_)
: Mod { name, std::move(config) }
{
  particleField.setup(600'000, ofFloatColor(1.0, 1.0, 1.0, 0.3), fieldValueOffset_);
}

void ParticleFieldMod::initParameters() {
  parameters.add(particleField.getParameterGroup());
}

void ParticleFieldMod::update() {
  auto fboPtr = fboPtrs[0];
  if (!fboPtr) return;
  
  particleField.update();
  particleField.draw(fboPtr->getSource());
}

void ParticleFieldMod::receive(int sinkId, const ofFloatPixels& value) {
  switch (sinkId) {
    case SINK_FIELD:
      particleField.setFieldTexture(value);
      break;
    default:
      ofLogError() << "ofPixels receive in " << typeid(*this).name() << " for unknown sinkId " << sinkId;
  }
}


} // ofxMarkSynth
