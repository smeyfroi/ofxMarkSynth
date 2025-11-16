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



class FluidMod;



// Allows us to use ParamController
class FluidSimulationAdaptor : public FluidSimulation {
public:
  FluidSimulationAdaptor(FluidMod* ownerModPtr) : ownerModPtr{ ownerModPtr } {};
  float getDt() const override;
  float getVorticity() const override;
  float getValueAdvectDissipation() const override;
  float getVelocityAdvectDissipation() const override;
//  int getValueDiffusionIterations() const override;
//  int getVelocityDiffusionIterations() const override;
//  int getPressureDiffusionIterations() const override;
private:
  FluidMod* ownerModPtr;
};



class FluidMod : public Mod {
  friend class FluidSimulationAdaptor;

public:
  FluidMod(Synth* synthPtr, const std::string& name, ModConfig config);
  float getAgency() const override;
  void update() override;
  void setup();
  void applyIntent(const Intent& intent, float strength) override;

  static constexpr std::string VELOCITIES_LAYERPTR_NAME { "velocities" };
  
protected:
  void initParameters() override;

private:
  FluidSimulationAdaptor fluidSimulation { this };
  
  std::unique_ptr<ParamController<float>> dtControllerPtr;
  std::unique_ptr<ParamController<float>> vorticityControllerPtr;
  std::unique_ptr<ParamController<float>> valueDissipationControllerPtr;
  std::unique_ptr<ParamController<float>> velocityDissipationControllerPtr;
  ofParameter<float> agencyFactorParameter { "Agency Factor", 1.0, 0.0, 1.0 }; // 0.0 -> No agency; 1.0 -> Global synth agency

};



} // ofxMarkSynth
