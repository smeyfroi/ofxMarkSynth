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
  void receive(int sinkId, const glm::vec4& v) override;
  void receive(int sinkId, const float& v) override;

  static constexpr int SINK_FIELD_1_FBO = 20;
  static constexpr int SINK_FIELD_2_FBO = 21;
  static constexpr int SINK_COLOR_FIELD_FBO = 30; // TODO: not implemented yet
  static constexpr int SINK_POINT_COLOR = 31; // to update color for a block of particles

protected:
  void initParameters() override;
  
private:
  ofxParticleField::ParticleField particleField;
  
};


}
