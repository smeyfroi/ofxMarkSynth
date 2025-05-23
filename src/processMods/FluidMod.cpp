//
//  FluidMod.cpp
//  example_collage
//
//  Created by Steve Meyfroidt on 22/05/2025.
//

#include "FluidMod.hpp"


namespace ofxMarkSynth {


FluidMod::FluidMod(const std::string& name, const ModConfig&& config)
: Mod { name, std::move(config) }
{}

void FluidMod::initParameters() {
  parameters.add(fluidSimulation.getParameterGroup());
}

void FluidMod::update() {
  if (!fluidSimulation.isSetup()) {
    if (fboPtrs[0] && fboPtrs[1]) {
      fluidSimulation.setup(fboPtrs[0], fboPtrs[1]);
    } else {
      return;
    }
  }
  fluidSimulation.update();
}


} // ofxMarkSynth
