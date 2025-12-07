//
//  FluidMod.hpp
//  example_collage
//
//  Created by Steve Meyfroidt on 22/05/2025.
//

#pragma once

#include <memory>
#include "Mod.hpp"
#include "FluidSimulation.h"
#include "ParamController.h"

namespace ofxMarkSynth {



class FluidMod : public Mod {

public:
  FluidMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config);
  float getAgency() const override;
  void update() override;
  void setup();
  void applyIntent(const Intent& intent, float strength) override;

  static constexpr int SOURCE_VELOCITIES_TEXTURE = 10;

  static constexpr std::string VELOCITIES_LAYERPTR_NAME { "velocities" };
  
protected:
  void initParameters() override;

private:
  FluidSimulation fluidSimulation;
  
  std::unique_ptr<ParamController<float>> dtControllerPtr;
  std::unique_ptr<ParamController<float>> vorticityControllerPtr;
  std::unique_ptr<ParamController<float>> valueDissipationControllerPtr;
  std::unique_ptr<ParamController<float>> velocityDissipationControllerPtr;
  ofParameter<float> agencyFactorParameter { "Agency Factor", 1.0, 0.0, 1.0 }; // 0.0 -> No agency; 1.0 -> Global synth agency

};



} // ofxMarkSynth
