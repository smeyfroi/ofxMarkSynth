//
//  FluidMod.hpp
//  example_collage
//
//  Created by Steve Meyfroidt on 22/05/2025.
//

#pragma once

#include "Mod.hpp"
#include "FluidSimulation.h"

namespace ofxMarkSynth {


class FluidMod : public Mod {

public:
  FluidMod(const std::string& name, const ModConfig&& config);
  void update() override;
  void setup();

  static constexpr int SINK_VALUES_FBO = SINK_FBOPTR;
  static constexpr int SINK_VELOCITIES_FBO = SINK_FBOPTR_2;
  
protected:
  void initParameters() override;

private:
  FluidSimulation fluidSimulation;

};


} // ofxMarkSynth
