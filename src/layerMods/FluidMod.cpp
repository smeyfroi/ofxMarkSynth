//
//  FluidMod.cpp
//  example_collage
//
//  Created by Steve Meyfroidt on 22/05/2025.
//

#include "FluidMod.hpp"

#include <algorithm>

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

  sinkNameIdMap = {
    { "TempImpulsePoint", SINK_TEMP_IMPULSE_POINT },
    { tempImpulseRadiusParameter.getName(), SINK_TEMP_IMPULSE_RADIUS },
    { tempImpulseDeltaParameter.getName(), SINK_TEMP_IMPULSE_DELTA },
  };

  registerControllerForSource(tempImpulseRadiusParameter, tempImpulseRadiusController);
  registerControllerForSource(tempImpulseDeltaParameter, tempImpulseDeltaController);
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

  // Temperature enable is part of FluidSimulation's parameter group (flattened).
  tempEnabledParamPtr = &group.getGroup("Temperature").get("TempEnabled").cast<bool>();

  parameters.add(tempImpulseRadiusParameter);
  parameters.add(tempImpulseDeltaParameter);
  
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
  auto obstaclesDrawingLayerPtr = getNamedDrawingLayerPtr(OBSTACLES_LAYERPTR_NAME, 0);

  if (!valuesDrawingLayerPtr || !velocitiesDrawingLayerPtr) {
    logValidationOnce("FluidMod '" + getName() + "': missing required drawing layers ('default' and 'velocities').");
    return;
  }

  if (obstaclesDrawingLayerPtr) {
    fluidSimulation.setup(valuesDrawingLayerPtr.value()->fboPtr,
                         velocitiesDrawingLayerPtr.value()->fboPtr,
                         obstaclesDrawingLayerPtr.value()->fboPtr);
  } else {
    fluidSimulation.setup(valuesDrawingLayerPtr.value()->fboPtr, velocitiesDrawingLayerPtr.value()->fboPtr);
  }
}

void FluidMod::update() {
  syncControllerAgencies();
  dtControllerPtr->update();
  vorticityControllerPtr->update();
  valueDissipationControllerPtr->update();
  velocityDissipationControllerPtr->update();
  tempImpulseRadiusController.update();
  tempImpulseDeltaController.update();

  auto drawingLayerPtrOpt = getCurrentNamedDrawingLayerPtr(DEFAULT_DRAWING_LAYER_PTR_NAME);
  if (!drawingLayerPtrOpt) {
    newTempImpulsePoints.clear();
    return;
  }

  setup();
  if (!fluidSimulation.isSetup()) {
    newTempImpulsePoints.clear();
    return;
  }

  const bool tempEnabled = (tempEnabledParamPtr != nullptr) ? tempEnabledParamPtr->get() : false;
  if (tempEnabled) tempSinksUsedWhileDisabledLogged = false;

  if (!newTempImpulsePoints.empty()) {
    if (!tempEnabled) {
      if (!tempSinksUsedWhileDisabledLogged) {
        tempSinksUsedWhileDisabledLogged = true;
        ofLogWarning("FluidMod") << "'" << getName() << "': TempImpulse sinks used but TempEnabled is false; enable 'TempEnabled' under Fluid Simulation > Temperature";
      }
      newTempImpulsePoints.clear();
    } else {
      const auto& velFbo = fluidSimulation.getFlowVelocitiesFbo().getSource();
      const float w = velFbo.getWidth();
      const float h = velFbo.getHeight();
      const float radiusPx = tempImpulseRadiusController.value * std::min(w, h);

      for (const auto& p : newTempImpulsePoints) {
        fluidSimulation.applyTemperatureImpulse({ p.x * w, p.y * h }, radiusPx, tempImpulseDeltaController.value);
      }

      newTempImpulsePoints.clear();
    }
  }

  fluidSimulation.update();
  if (!fluidSimulation.isValid()) {
    logValidationOnce("FluidMod '" + getName() + "': " + fluidSimulation.getValidationError());
    return;
  }

  logValidationOnce("");
  emit(SOURCE_VELOCITIES_TEXTURE, fluidSimulation.getFlowVelocitiesFbo().getSource().getTexture());
}

void FluidMod::receive(int sinkId, const float& value) {
  if (!canDrawOnNamedLayer()) return;

  const bool tempEnabled = (tempEnabledParamPtr != nullptr) ? tempEnabledParamPtr->get() : false;
  if (!tempEnabled && !tempSinksUsedWhileDisabledLogged) {
    tempSinksUsedWhileDisabledLogged = true;
    ofLogWarning("FluidMod") << "'" << getName() << "': TempImpulse sinks used but TempEnabled is false; enable 'TempEnabled' under Fluid Simulation > Temperature";
  }

  switch (sinkId) {
    case SINK_TEMP_IMPULSE_RADIUS:
      tempImpulseRadiusController.updateAuto(value, getAgency());
      break;
    case SINK_TEMP_IMPULSE_DELTA:
      tempImpulseDeltaController.updateAuto(value, getAgency());
      break;
    default:
      ofLogError("FluidMod") << "Float receive for unknown sinkId " << sinkId;
  }
}

void FluidMod::receive(int sinkId, const glm::vec2& point) {
  if (!canDrawOnNamedLayer()) return;

  if (sinkId != SINK_TEMP_IMPULSE_POINT) {
    ofLogError("FluidMod") << "glm::vec2 receive for unknown sinkId " << sinkId;
    return;
  }

  const bool tempEnabled = (tempEnabledParamPtr != nullptr) ? tempEnabledParamPtr->get() : false;
  if (!tempEnabled) {
    if (!tempSinksUsedWhileDisabledLogged) {
      tempSinksUsedWhileDisabledLogged = true;
      ofLogWarning("FluidMod") << "'" << getName() << "': TempImpulse sinks used but TempEnabled is false; enable 'TempEnabled' under Fluid Simulation > Temperature";
    }
    return;
  }

  newTempImpulsePoints.push_back(point);
}

void FluidMod::applyIntent(const Intent& intent, float strength) {
  if (!fluidSimulation.isValid()) return;

  IntentMap im(intent);
  im.E().exp(*dtControllerPtr, strength);

  // High Structure should read as more ordered/laminar flow.
  // Keep Chaos as the primary vorticity driver, but attenuate it as S rises.
  float vorticityDim = im.C().get() * (1.0f - im.S().get() * 0.75f); // S=1 -> 25% of Chaos
  vorticityDim = std::clamp(vorticityDim, 0.0f, 1.0f);
  float vorticityI = linearMap(vorticityDim, vorticityControllerPtr->getManualMin(), vorticityControllerPtr->getManualMax());
  vorticityControllerPtr->updateIntent(vorticityI, strength, "C*(1-0.75*S) -> lin");

  im.D().inv().lin(*valueDissipationControllerPtr, strength);
  im.G().inv().exp(*velocityDissipationControllerPtr, strength);
}
} // ofxMarkSynth
