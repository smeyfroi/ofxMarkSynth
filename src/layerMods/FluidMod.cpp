//
//  FluidMod.cpp
//  example_collage
//
//  Created by Steve Meyfroidt on 22/05/2025.
//

#include "FluidMod.hpp"
#include "Intent.hpp"
#include "IntentMapping.hpp"



namespace ofxMarkSynth {



FluidMod::FluidMod(Synth* synthPtr, const std::string& name, const ModConfig&& config)
: Mod { synthPtr, name, std::move(config) }
{}

float FluidMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

void FluidMod::initParameters() {
  auto& group = fluidSimulation.getParameterGroup();
  parameters.add(group);
  
  dtController = std::make_unique<ParamController<float>>(group.get("dt").cast<float>());
  vorticityController = std::make_unique<ParamController<float>>(group.get("Vorticity").cast<float>());
  valueDissipationController = std::make_unique<ParamController<float>>(group.get("Value Dissipation").cast<float>());
  velocityDissipationController = std::make_unique<ParamController<float>>(group.get("Velocity Dissipation").cast<float>());
}

void FluidMod::setup() {
  if (!fluidSimulation.isSetup()) {
    auto valuesDrawingLayerPtr = getNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME, 0); // getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
    auto velocitiesDrawingLayerPtr = getNamedDrawingLayerPtr(VELOCITIES_LAYERPTR_NAME, 0); // special case for velocities
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

void FluidMod::applyIntent(const Intent& intent, float strength) {
  if (strength < 0.01) return;
  if (!fluidSimulation.isSetup()) return;
  
  // Energy → dt
  float dtI = exponentialMap(intent.getEnergy(), dtController->getManualMin(), dtController->getManualMax());
  dtController->updateIntent(dtI, strength);
  
  // Chaos → Vorticity
  float vorticityI = linearMap(intent.getChaos(), vorticityController->getManualMin(), vorticityController->getManualMax());
  vorticityController->updateIntent(vorticityI, strength);
  
  // Density → Value Dissipation
  float valueDissipationI = inverseMap(intent.getDensity(), valueDissipationController->getManualMin(), valueDissipationController->getManualMax());
  valueDissipationController->updateIntent(valueDissipationI, strength);
  
  // Granularity → Velocity Dissipation
  float velocityDissipationI = inverseMap(intent.getGranularity(), velocityDissipationController->getManualMin(), velocityDissipationController->getManualMax());
  velocityDissipationController->updateIntent(velocityDissipationI, strength);
}



} // ofxMarkSynth
