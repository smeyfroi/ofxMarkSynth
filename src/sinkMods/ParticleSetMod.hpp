//
//  ParticleSetMod.hpp
//  example_particles
//
//  Created by Steve Meyfroidt on 09/05/2025.
//

#pragma once

#include <memory>
#include <vector>
#include "ofxGui.h"
#include "core/Mod.hpp"
#include "core/ColorRegister.hpp"
#include "ofxParticleSet.h"
#include "core/ParamController.h"
//#include "AddTextureThresholded.h"


namespace ofxMarkSynth {



class ParticleSetMod : public Mod {

public:
  ParticleSetMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config);
  float getAgency() const override;
  void update() override;
  void receive(int sinkId, const float& value) override;
  void receive(int sinkId, const glm::vec2& point) override;
  void receive(int sinkId, const glm::vec4& v) override;
  void applyIntent(const Intent& intent, float strength) override;

  static constexpr int SINK_POINT = 1;
  static constexpr int SINK_CHANGE_KEY_COLOUR = 90;
  static constexpr int SINK_POINT_VELOCITY = 2;
  static constexpr int SINK_SPIN = 10;
  static constexpr int SINK_COLOR = 20;
  static constexpr int SINK_ALPHA_MULTIPLIER = 30;

protected:
  void initParameters() override;

private:
  ParticleSet particleSet;

  ofParameter<float> spinParameter { "Spin", 0.03, -0.05, 0.05 };
  ParamController<float> spinController { spinParameter };
  ofParameter<ofFloatColor> colorParameter { "Colour", ofFloatColor(1.0, 1.0, 1.0, 1.0), ofFloatColor(0.0, 0.0, 0.0, 0.0), ofFloatColor(1.0, 1.0, 1.0, 1.0) };
  ParamController<ofFloatColor> colorController { colorParameter };

  // Extra alpha scaling for new particles. Useful when layer persistence changes.
  ofParameter<float> alphaMultiplierParameter { "AlphaMultiplier", 1.0f, 0.0f, 4.0f };
  ParamController<float> alphaMultiplierController { alphaMultiplierParameter };

    // Key colour register: pipe-separated vec4 list.
  // Example: "0,0,0,0.3 | 0.5,0.5,0.5,0.3 | 1,1,1,0.3"
  ofParameter<std::string> keyColoursParameter { "KeyColours", "" };
  ColorRegister keyColourRegister;
  bool keyColourRegisterInitialized { false };

  std::unique_ptr<ParamController<float>> timeStepControllerPtr;
  std::unique_ptr<ParamController<float>> velocityDampingControllerPtr;
  std::unique_ptr<ParamController<float>> attractionStrengthControllerPtr;
  std::unique_ptr<ParamController<float>> attractionRadiusControllerPtr;
  std::unique_ptr<ParamController<float>> forceScaleControllerPtr;
  std::unique_ptr<ParamController<float>> connectionRadiusControllerPtr;
  std::unique_ptr<ParamController<float>> colourMultiplierControllerPtr;
  std::unique_ptr<ParamController<float>> maxSpeedControllerPtr;

  ParticleSet::ParameterOverrides lastAppliedParameterOverrides;
  bool hasLastAppliedParameterOverrides = false;

  ofParameter<float> agencyFactorParameter { "AgencyFactor", 1.0, 0.0, 1.0 }; // 0.0 -> No agency; 1.0 -> Global synth agency

  std::vector<glm::vec4> newPoints; // { x, y, dx, dy }
};


} // ofxMarkSynth
