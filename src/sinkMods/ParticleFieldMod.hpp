//
//  ParticleFieldMod.hpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 10/09/2025.
//

#pragma once

#include "Mod.hpp"
#include "ofxParticleField.h"

namespace ofxMarkSynth {


class ParticleFieldMod : public Mod {
  
public:
  ParticleFieldMod(const std::string& name, const ModConfig&& config, float fieldValueOffset_ = 0.5, int particleCount_ = 500'000);
  void update() override;
  void receive(int sinkId, const ofFbo& value) override;

  static constexpr int SINK_FIELD_FBO = 11;
  
protected:
  void initParameters() override;
  
private:
  ofxParticleField::ParticleField particleField;
  
};


}
