//
//  FluidMod.cpp
//  example_collage
//
//  Created by Steve Meyfroidt on 22/05/2025.
//

#include "FluidMod.hpp"
#include "Intent.hpp"
#include "IntentMapping.hpp"
#include "Parameter.hpp"



namespace ofxMarkSynth {



float FluidSimulationAdaptor::getDt() const {
  return static_cast<float>(ownerModPtr->dtControllerPtr->value);
}
float FluidSimulationAdaptor::getVorticity() const {
  return static_cast<float>(ownerModPtr->vorticityControllerPtr->value);
}
float FluidSimulationAdaptor::getValueAdvectDissipation() const {
  return static_cast<float>(ownerModPtr->valueDissipationControllerPtr->value);
}
float FluidSimulationAdaptor::getVelocityAdvectDissipation() const {
  return static_cast<float>(ownerModPtr->velocityDissipationControllerPtr->value);
}



FluidMod::FluidMod(Synth* synthPtr, const std::string& name, const ModConfig&& config)
: Mod { synthPtr, name, std::move(config) }
{}

float FluidMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}

void FluidMod::initParameters() {
  auto& group = fluidSimulation.getParameterGroup();
  addFlattenedParameterGroup(parameters, group);
  parameters.add(agencyFactorParameter);
  
  dtControllerPtr = std::make_unique<ParamController<float>>(group.get("dt").cast<float>());
  vorticityControllerPtr = std::make_unique<ParamController<float>>(group.get("Vorticity").cast<float>());
  valueDissipationControllerPtr = std::make_unique<ParamController<float>>(group.get("Value Dissipation").cast<float>());
  velocityDissipationControllerPtr = std::make_unique<ParamController<float>>(group.get("Velocity Dissipation").cast<float>());
  
  sourceNameControllerPtrMap = {
    { group.get("dt").cast<float>().getName(), dtControllerPtr.get() },
    { group.get("Vorticity").cast<float>().getName(), vorticityControllerPtr.get() },
    { group.get("Value Dissipation").cast<float>().getName(), valueDissipationControllerPtr.get() },
    { group.get("Velocity Dissipation").cast<float>().getName(), velocityDissipationControllerPtr.get() }
  };
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
  float dtI = exponentialMap(intent.getEnergy(), *dtControllerPtr);
  dtControllerPtr->updateIntent(dtI, strength);
  
  // Chaos → Vorticity
  float vorticityI = linearMap(intent.getChaos(), *vorticityControllerPtr);
  vorticityControllerPtr->updateIntent(vorticityI, strength);
  
  // Inverse Density → Value Dissipation
  float valueDissipationI = inverseMap(intent.getDensity(), *valueDissipationControllerPtr);
  valueDissipationControllerPtr->updateIntent(valueDissipationI, strength);
  
  // Inverse Granularity → Velocity Dissipation
  float velocityDissipationI = inverseExponentialMap(intent.getGranularity(),
                                                     *velocityDissipationControllerPtr);
  velocityDissipationControllerPtr->updateIntent(velocityDissipationI, strength);
}



} // ofxMarkSynth
