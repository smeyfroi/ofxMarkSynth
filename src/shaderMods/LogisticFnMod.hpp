//
//  LogisticFnMod.hpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 06/06/2025.
//

#pragma once

#include "ofxGui.h"
#include "Mod.hpp"
#include "PingPongFbo.h"
#include "LogisticFnShader.h"


namespace ofxMarkSynth {


class LogisticFnMod : public Mod {

public:
  LogisticFnMod(const std::string& name, const ModConfig&& config);
  void update() override;

protected:
  void initParameters() override;

private:
  ofParameter<float> clampFactorParameter { "clampFactor", 1.0, 0.0, 1.0 };

  LogisticFnShader logisticFnShader;
};


} // ofxMarkSynth
