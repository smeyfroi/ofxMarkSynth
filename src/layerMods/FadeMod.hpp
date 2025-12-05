//
//  FadeMod.hpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 29/10/2025.
//

#pragma once

#include "Mod.hpp"
#include "ofxGui.h"
#include "UnitQuadMesh.h"
#include "ParamController.h"
#include "IntentMapping.hpp"



namespace ofxMarkSynth {



class FadeMod : public Mod {
public:
  FadeMod(Synth* synthPtr, const std::string& name, ModConfig config);
  float getAgency() const override;
  void update() override;
  void receive(int sinkId, const float& value) override;
  void applyIntent(const Intent& intent, float strength) override;

  static constexpr int SINK_ALPHA_MULTIPLIER = 11;
  
protected:
  void initParameters() override;
  
private:
  UnitQuadMesh unitQuadMesh;

  ofParameter<float> alphaMultiplierParameter { "Alpha", 0.01, 0.0, 0.1 };
  ParamController<float> alphaMultiplierController { alphaMultiplierParameter };
  ofParameter<float> agencyFactorParameter { "AgencyFactor", 1.0, 0.0, 1.0 };
};



} // ofxMarkSynth
