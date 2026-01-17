//
//  FluidMod.cpp
//  example_collage
//
//  Created by Steve Meyfroidt on 22/05/2025.
//

#include "FluidMod.hpp"
#include "core/Intent.hpp"
#include "core/IntentMapper.hpp"
#include "core/IntentMapping.hpp"
#include "config/Parameter.hpp"



namespace ofxMarkSynth {



FluidMod::FluidMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config)
: Mod { synthPtr, name, std::move(config) }
{
  sourceNameIdMap = {
    { "velocitiesTexture", SOURCE_VELOCITIES_TEXTURE }
  };
}

void FluidMod::logValidationOnce(const std::string& message) {
  if (message.empty()) {
    lastValidationLog.clear();
    return;
  }

  if (message == lastValidationLog) return;

  lastValidationLog = message;
  ofLogError("FluidMod") << message;
}

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
  
  auto& dtParam = group.get("dt").cast<float>();
  auto& vorticityParam = group.get("Vorticity").cast<float>();
  auto& valueDissipationParam = group.get("Value Dissipation").cast<float>();
  auto& velocityDissipationParam = group.get("Velocity Dissipation").cast<float>();
  registerControllerForSource(dtParam, *dtControllerPtr);
  registerControllerForSource(vorticityParam, *vorticityControllerPtr);
  registerControllerForSource(valueDissipationParam, *valueDissipationControllerPtr);
  registerControllerForSource(velocityDissipationParam, *velocityDissipationControllerPtr);
}

void FluidMod::setup() {
  if (fluidSimulation.isSetup()) return;

  auto valuesDrawingLayerPtr = getNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME, 0);
  auto velocitiesDrawingLayerPtr = getNamedDrawingLayerPtr(VELOCITIES_LAYERPTR_NAME, 0);

  if (!valuesDrawingLayerPtr || !velocitiesDrawingLayerPtr) {
    logValidationOnce("FluidMod '" + getName() + "': missing required drawing layers ('default' and 'velocities').");
    return;
  }

  fluidSimulation.setup(valuesDrawingLayerPtr.value()->fboPtr, velocitiesDrawingLayerPtr.value()->fboPtr);
}

void FluidMod::update() {
  syncControllerAgencies();
  dtControllerPtr->update();
  vorticityControllerPtr->update();
  valueDissipationControllerPtr->update();
  velocityDissipationControllerPtr->update();
  
  auto drawingLayerPtrOpt = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) return;
  
  setup();
  if (!fluidSimulation.isSetup()) return;

  fluidSimulation.update();
  if (!fluidSimulation.isValid()) {
    logValidationOnce("FluidMod '" + getName() + "': " + fluidSimulation.getValidationError());
    return;
  }

  logValidationOnce("");
  emit(SOURCE_VELOCITIES_TEXTURE, fluidSimulation.getFlowVelocitiesFbo().getSource().getTexture());
}

void FluidMod::applyIntent(const Intent& intent, float strength) {
  if (!fluidSimulation.isValid()) return;
  
  IntentMap im(intent);
  im.E().exp(*dtControllerPtr, strength);
  im.C().lin(*vorticityControllerPtr, strength);
  im.D().inv().lin(*valueDissipationControllerPtr, strength);
  im.G().inv().exp(*velocityDissipationControllerPtr, strength);
}



} // ofxMarkSynth
