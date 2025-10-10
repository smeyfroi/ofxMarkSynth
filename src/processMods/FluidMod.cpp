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

void FluidMod::setup() {
  if (!fluidSimulation.isSetup()) {
    auto valuesFboPtr = getNamedFboPtr(DEFAULT_FBOPTR_NAME);
    auto velocitiesFboPtr = getNamedFboPtr(VELOCITIES_FBOPTR_NAME);
    if (valuesFboPtr && velocitiesFboPtr) {
      fluidSimulation.setup(valuesFboPtr.value(), velocitiesFboPtr.value());
    } else {
      return;
    }
  }
}

void FluidMod::update() {
  setup();  
  fluidSimulation.update();
}


} // ofxMarkSynth
