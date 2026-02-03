//
//  FadeAlphaMapMod.hpp
//  ofxMarkSynth
//
//  Created by Steve Meyfroidt on 02/02/2026.
//

#pragma once

#include "core/Mod.hpp"
#include "core/ParamController.h"

namespace ofxMarkSynth {

// Maps an arbitrary float input to a Fade half-life in seconds.
//
// Typical use: map Audio.RmsScalar to layer persistence.
// Historically this was done via MultiplyAdd into Fade.Alpha (alpha-per-frame).
// This Mod keeps the same linear mapping (Multiplier/Adder) but outputs HalfLifeSec,
// using the reference FPS to interpret the legacy alpha-per-frame signal.
class FadeAlphaMapMod : public Mod {
public:
  FadeAlphaMapMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config);
  float getAgency() const override;
  void update() override;
  void receive(int sinkId, const float& value) override;

  static constexpr int SINK_MULTIPLIER = 10;
  static constexpr int SINK_ADDER = 11;
  static constexpr int SINK_FLOAT = 20;
  static constexpr int SOURCE_FLOAT = 30;

protected:
  void initParameters() override;

private:
  float mapAlphaToHalfLifeSec(float alphaPerFrame) const;

  ofParameter<float> multiplierParameter { "Multiplier", 1.0f, -2.0f, 2.0f };
  ParamController<float> multiplierController { multiplierParameter };
  ofParameter<float> adderParameter { "Adder", 0.0f, -1.0f, 1.0f };
  ParamController<float> adderController { adderParameter };

  // Reference FPS used to interpret the legacy alpha-per-frame mapping.
  ofParameter<float> referenceFpsParameter { "ReferenceFps", 30.0f, 1.0f, 240.0f };

  ofParameter<float> minHalfLifeSecParameter { "MinHalfLifeSec", 0.05f, 0.001f, 10.0f };
  ofParameter<float> maxHalfLifeSecParameter { "MaxHalfLifeSec", 300.0f, 1.0f, 3600.0f };

  ofParameter<float> agencyFactorParameter { "AgencyFactor", 1.0f, 0.0f, 1.0f };
};

} // namespace ofxMarkSynth
