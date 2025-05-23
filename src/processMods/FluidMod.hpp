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

  static constexpr int SINK_VALUES_FBO = SINK_FBO;
  static constexpr int SINK_VELOCITIES_FBO = SINK_FBO_2;
  
protected:
  void initParameters() override;

private:
//  ofParameter<float> snapshotsPerUpdateParameter { "SnapshotsPerUpdate", 1.0/30.0, 0.0, 1.0 };
  FluidSimulation fluidSimulation;

};


} // ofxMarkSynth

