//
//  FluidMod.hpp
//  example_collage
//
//  Created by Steve Meyfroidt on 22/05/2025.
//

#pragma once

#include <memory>
#include <vector>

#include "core/Mod.hpp"
#include "FluidSimulation.h"
#include "core/ParamController.h"

namespace ofxMarkSynth {



class FluidMod : public Mod {
 
public:
  FluidMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config);
  float getAgency() const override;
  void update() override;
  void setup();
  void receive(int sinkId, const float& value) override;
  void receive(int sinkId, const glm::vec2& point) override;
  void applyIntent(const Intent& intent, float strength) override;
 
  static constexpr int SOURCE_VELOCITIES_TEXTURE = 10;

  static constexpr int SINK_TEMP_IMPULSE_POINT = 100;
  static constexpr int SINK_TEMP_IMPULSE_RADIUS = 110;
  static constexpr int SINK_TEMP_IMPULSE_DELTA = 120;
 
  static constexpr std::string VELOCITIES_LAYERPTR_NAME { "velocities" };
  static constexpr std::string OBSTACLES_LAYERPTR_NAME { "obstacles" };
  
protected:
  void initParameters() override;

private:
  void logValidationOnce(const std::string& message);

  FluidSimulation fluidSimulation;
  
  std::unique_ptr<ParamController<float>> dtControllerPtr;
  std::unique_ptr<ParamController<float>> vorticityControllerPtr;
  std::unique_ptr<ParamController<float>> valueDissipationControllerPtr;
  std::unique_ptr<ParamController<float>> velocityDissipationControllerPtr;
  ofParameter<float> agencyFactorParameter { "AgencyFactor", 1.0, 0.0, 1.0 }; // 0.0 -> No agency; 1.0 -> Global synth agency

  ofParameter<float> tempImpulseRadiusParameter { "TempImpulseRadius", 0.03f, 0.0f, 0.10f };
  ParamController<float> tempImpulseRadiusController { tempImpulseRadiusParameter };
  ofParameter<float> tempImpulseDeltaParameter { "TempImpulseDelta", 0.6f, -1.0f, 1.0f };
  ParamController<float> tempImpulseDeltaController { tempImpulseDeltaParameter };

  std::vector<glm::vec2> newTempImpulsePoints;
  ofParameter<bool>* tempEnabledParamPtr { nullptr };
  bool tempSinksUsedWhileDisabledLogged { false };

  // Tracks the last fatal setup/validation error we logged, to avoid per-frame spam.
  std::string lastValidationLog;

};



} // ofxMarkSynth
