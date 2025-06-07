//
//  ClampMod.hpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 07/06/2025.
//

#pragma once

#include "ofxGui.h"
#include "Mod.hpp"
#include "PingPongFbo.h"
#include "ClampShader.h"


namespace ofxMarkSynth {


class ClampMod : public Mod {

public:
  ClampMod(const std::string& name, const ModConfig&& config);
  void update() override;

protected:
  void initParameters() override;

private:
  ClampShader clampShader;
};


} // ofxMarkSynth
