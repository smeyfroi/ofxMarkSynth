//
//  ParticleFieldMod.hpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 10/09/2025.
//

#pragma once

#include <memory>
#include "Mod.hpp"
#include "ofxParticleField.h"
#include "ParamController.h"

namespace ofxMarkSynth {



class ParticleFieldMod : public Mod {
  
public:
  ParticleFieldMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config, float field1Bias_ = 0.0, float field2Bias_ = 0.0);
  float getAgency() const override;
  void update() override;
  void receive(int sinkId, const ofTexture& value) override;
  void receive(int sinkId, const glm::vec4& v) override;
  void receive(int sinkId, const float& v) override;
  void applyIntent(const Intent& intent, float strength) override;

  static constexpr int SINK_FIELD_1_FBO = 20;
  static constexpr int SINK_FIELD_2_FBO = 21;
  static constexpr int SINK_COLOR_FIELD_FBO = 30; // TODO: not implemented yet
  static constexpr int SINK_POINT_COLOR = 31; // to update color for a block of particles
  static constexpr int SINK_MIN_WEIGHT = 40;
  static constexpr int SINK_MAX_WEIGHT = 41;

protected:
  void initParameters() override;
  
private:
  ofxParticleField::ParticleField particleField;
  ofParameter<float> agencyFactorParameter { "AgencyFactor", 1.0, 0.0, 1.0 };
  ofParameter<ofFloatColor> pointColorParameter {
    "PointColour",
    ofFloatColor { 1.0f, 1.0f, 1.0f, 0.3f },
    ofFloatColor { 0.0f, 0.0f, 0.0f, 0.0f },
    ofFloatColor { 1.0f, 1.0f, 1.0f, 1.0f }
  };
  
  std::unique_ptr<ParamController<float>> minWeightControllerPtr;
  std::unique_ptr<ParamController<float>> maxWeightControllerPtr;
  std::unique_ptr<ParamController<ofFloatColor>> pointColorControllerPtr;
};



} // ofxMarkSynth
