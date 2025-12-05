//
//  RandomHslColorMod.hpp
//  example_sandlines
//
//  Created by Steve Meyfroidt on 10/05/2025.
//

#pragma once

#include "Mod.hpp"
#include "ParamController.h"

namespace ofxMarkSynth {


class RandomHslColorMod : public Mod {

public:
  RandomHslColorMod(Synth* synthPtr, const std::string& name, ModConfig config);
  float getAgency() const override;
  void update() override;
  void applyIntent(const Intent& intent, float strength) override;
  
  static constexpr int SOURCE_VEC4 = 1;
  
  static constexpr int SINK_COLORS_PER_UPDATE = 1;
  static constexpr int SINK_HUE_CENTER = 2;
  static constexpr int SINK_HUE_WIDTH = 3;
  static constexpr int SINK_MIN_SATURATION = 4;
  static constexpr int SINK_MAX_SATURATION = 5;
  static constexpr int SINK_MIN_BRIGHTNESS = 6;
  static constexpr int SINK_MAX_BRIGHTNESS = 7;
  static constexpr int SINK_MIN_ALPHA = 8;
  static constexpr int SINK_MAX_ALPHA = 9;

protected:
  void initParameters() override;
  void receive(int sinkId, const float& value) override;

private:
  float colorCount;
  const ofFloatColor createRandomColor() const;
  float randomHueFromCenterWidth(float center, float width) const;
  ofParameter<float> colorsPerUpdateParameter { "CreatedPerUpdate", 1.0, 0.0, 100.0 };
  ParamController<float> colorsPerUpdateController { colorsPerUpdateParameter };
  ofParameter<float> hueCenterParameter { "HueCenter", 0.0, 0.0, 1.0 };
  ParamController<float> hueCenterController { hueCenterParameter, true /* angular */ };
  ofParameter<float> hueWidthParameter { "HueWidth", 0.1, 0.0, 1.0 };
  ParamController<float> hueWidthController { hueWidthParameter };
  ofParameter<float> minSaturationParameter { "MinSaturation", 0.0, 0.0, 1.0 };
  ParamController<float> minSaturationController { minSaturationParameter };
  ofParameter<float> maxSaturationParameter { "MaxSaturation", 1.0, 0.0, 1.0 };
  ParamController<float> maxSaturationController { maxSaturationParameter };
  ofParameter<float> minBrightnessParameter { "MinBrightness", 0.0, 0.0, 1.0 };
  ParamController<float> minBrightnessController { minBrightnessParameter };
  ofParameter<float> maxBrightnessParameter { "MaxBrightness", 1.0, 0.0, 1.0 };
  ParamController<float> maxBrightnessController { maxBrightnessParameter };
  ofParameter<float> minAlphaParameter { "MinAlpha", 0.0, 0.0, 1.0 };
  ParamController<float> minAlphaController { minAlphaParameter };
  ofParameter<float> maxAlphaParameter { "MaxAlpha", 1.0, 0.0, 1.0 };
  ParamController<float> maxAlphaController { maxAlphaParameter };
  ofParameter<float> agencyFactorParameter { "AgencyFactor", 1.0, 0.0, 1.0 };
};


} // ofxMarkSynth
