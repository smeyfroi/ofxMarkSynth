//
//  ParticleSetMod.hpp
//  example_particles
//
//  Created by Steve Meyfroidt on 09/05/2025.
//

#pragma once

#include "ofxGui.h"
#include "Mod.hpp"
#include "ofxParticleSet.h"
#include "ParamController.h"
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
  static constexpr int SINK_POINT_VELOCITY = 2;
  static constexpr int SINK_SPIN = 10;
  static constexpr int SINK_COLOR = 20;

protected:
  void initParameters() override;

private:
  ParticleSet particleSet;

  ofParameter<float> spinParameter { "Spin", 0.03, -0.05, 0.05 };
  ParamController<float> spinController { spinParameter };
  ofParameter<ofFloatColor> colorParameter { "Colour", ofFloatColor(1.0, 1.0, 1.0, 1.0), ofFloatColor(0.0, 0.0, 0.0, 0.0), ofFloatColor(1.0, 1.0, 1.0, 1.0) };
  ParamController<ofFloatColor> colorController { colorParameter };

  std::unique_ptr<ParamController<float>> timeStepControllerPtr;
  std::unique_ptr<ParamController<float>> velocityDampingControllerPtr;
  std::unique_ptr<ParamController<float>> attractionStrengthControllerPtr;
  std::unique_ptr<ParamController<float>> attractionRadiusControllerPtr;
  std::unique_ptr<ParamController<float>> forceScaleControllerPtr;
  std::unique_ptr<ParamController<float>> connectionRadiusControllerPtr;
  std::unique_ptr<ParamController<float>> colourMultiplierControllerPtr;
  std::unique_ptr<ParamController<float>> maxSpeedControllerPtr;
  ofParameter<float> agencyFactorParameter { "Agency Factor", 1.0, 0.0, 1.0 }; // 0.0 -> No agency; 1.0 -> Global synth agency

  std::vector<glm::vec4> newPoints; // { x, y, dx, dy }
};


} // ofxMarkSynth
