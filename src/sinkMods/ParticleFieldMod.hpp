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
  ParticleFieldMod(const std::string& name, const ModConfig&& config, float field1Bias_ = -0.5, float field2Bias_ = -0.5);
  void update() override;
  void receive(int sinkId, const ofFbo& value) override;

  static constexpr int SINK_FIELD_1_FBO = 20;
  static constexpr int SINK_FIELD_2_FBO = 21;

protected:
  void initParameters() override;
  
private:
  ofxParticleField::ParticleField particleField;
  
};


}
