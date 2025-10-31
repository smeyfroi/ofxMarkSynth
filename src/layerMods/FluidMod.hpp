//
//  FluidMod.hpp
//  example_collage
//
//  Created by Steve Meyfroidt on 22/05/2025.
//

#pragma once

#include "Mod.hpp"
#include "FluidSimulation.h"
#include "ParamController.h"

namespace ofxMarkSynth {


class FluidMod : public Mod {

public:
  FluidMod(Synth* synthPtr, const std::string& name, const ModConfig&& config);
  float getAgency() const override;
  void update() override;
  void setup();
  void applyIntent(const Intent& intent, float strength) override;

  static constexpr std::string VELOCITIES_LAYERPTR_NAME { "velocities" };
  
protected:
  void initParameters() override;

private:
  FluidSimulation fluidSimulation;
  
  std::unique_ptr<ParamController<float>> dtController;
  std::unique_ptr<ParamController<float>> vorticityController;
  std::unique_ptr<ParamController<float>> valueDissipationController;
  std::unique_ptr<ParamController<float>> velocityDissipationController;
  ofParameter<float> agencyFactorParameter { "Agency Factor", 1.0, 0.0, 1.0 }; // 0.0 -> No agency; 1.0 -> Global synth agency

};


} // ofxMarkSynth
