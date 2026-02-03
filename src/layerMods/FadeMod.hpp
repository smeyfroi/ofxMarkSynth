//
//  FadeMod.hpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 29/10/2025.
//

#pragma once

#include "core/Mod.hpp"
#include "ofxGui.h"
#include "UnitQuadMesh.h"
#include "core/ParamController.h"
#include "core/IntentMapping.hpp"



namespace ofxMarkSynth {



class FadeMod : public Mod {
public:
  FadeMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config);
  float getAgency() const override;
  void update() override;
  void receive(int sinkId, const float& value) override;
  void applyIntent(const Intent& intent, float strength) override;

  static constexpr int SINK_HALF_LIFE_SEC = 11;
  static constexpr int SINK_ALPHA_LEGACY = 12;
  
protected:
  void initParameters() override;
  
private:
  UnitQuadMesh unitQuadMesh;

  // Time-based fade control.
  // HalfLifeSec = time for the layer to reach 50% intensity (RGBA multiplied).
  ofParameter<float> halfLifeSecParameter { "HalfLifeSec", 23.1f, 0.05f, 300.0f };
  ParamController<float> halfLifeSecController { halfLifeSecParameter };
  ofParameter<float> agencyFactorParameter { "AgencyFactor", 1.0, 0.0, 1.0 };
};



} // ofxMarkSynth
