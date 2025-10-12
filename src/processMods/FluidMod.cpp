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
    auto valuesDrawingLayerPtr = getNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
    auto velocitiesDrawingLayerPtr = getNamedDrawingLayerPtr(VELOCITIES_LAYERPTR_NAME);
    if (valuesDrawingLayerPtr && velocitiesDrawingLayerPtr) {
      fluidSimulation.setup(valuesDrawingLayerPtr.value()->fboPtr, velocitiesDrawingLayerPtr.value()->fboPtr);
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
