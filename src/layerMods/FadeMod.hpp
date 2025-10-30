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
#include "IntentParamController.h"
#include "IntentMapping.hpp"



namespace ofxMarkSynth {



class FadeMod : public Mod {
public:
  FadeMod(Synth* synthPtr, const std::string& name, const ModConfig&& config);
  void update() override;
  void receive(int sinkId, const float& value) override;
  void applyIntent(const Intent& intent, float strength) override;

  static constexpr int SINK_ALPHA_MULTIPLIER = 11;
  
protected:
  void initParameters() override;
  
private:
  UnitQuadMesh unitQuadMesh;

  ofParameter<float> alphaMultiplierParameter { "Alpha", 0.01, 0.0, 0.05 };
  IntentParamController<float> alphaMultiplierController { alphaMultiplierParameter };
};



} // ofxMarkSynth
