//
//  SmearMod.hpp
//  fingerprint-ipad-1
//
//  Created by Steve Meyfroidt on 01/08/2025.
//

#pragma once

#include "ofxGui.h"
#include "Mod.hpp"
#include "PingPongFbo.h"
#include "SmearShader.h"
#include "ParamController.h"


namespace ofxMarkSynth {


class SmearMod : public Mod {
public:
  void applyIntent(const Intent& intent, float strength) override;

public:
  SmearMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config);
  float getAgency() const override;
  void update() override;
  void receive(int sinkId, const glm::vec2& v) override;
  void receive(int sinkId, const float& value) override;
  void receive(int sinkId, const ofTexture& value) override;

  static constexpr int SINK_VEC2 = 10;
  static constexpr int SINK_FLOAT = 11;
  static constexpr int SINK_FIELD_1_TEX = 20;
  static constexpr int SINK_FIELD_2_TEX = 21;

protected:
  void initParameters() override;

private:
  ofParameter<float> mixNewParameter { "MixNew", 0.9, 0.3, 1.0 };
  ParamController<float> mixNewController { mixNewParameter };
  ofParameter<float> alphaMultiplierParameter { "AlphaMultiplier", 0.998, 0.994, 0.999 };
  ParamController<float> alphaMultiplierController { alphaMultiplierParameter };
//  ofParameter<glm::vec2> translateByParameter { "Translation", glm::vec2 { 0.0, 0.001 }, glm::vec2 { -0.01, -0.01 }, glm::vec2 { 0.01, 0.01 } };
  ofParameter<glm::vec2> translateByParameter { "Translation", glm::vec2 { 0.0, 0.0 }, glm::vec2 { -0.01, -0.01 }, glm::vec2 { 0.01, 0.01 } };
  ofParameter<float> field1MultiplierParameter { "Field1Multiplier", 0.001, 0.0, 0.05 };
  ParamController<float> field1MultiplierController { field1MultiplierParameter };
//  ofParameter<glm::vec2> field1BiasParameter { "Field1Bias", glm::vec2 { -0.5, -0.5 }, glm::vec2 { -1.0, -1.0 }, glm::vec2 { 1.0, 1.0 } };
  ofParameter<glm::vec2> field1BiasParameter { "Field1Bias", glm::vec2 { 0.0, 0.0 }, glm::vec2 { -1.0, -1.0 }, glm::vec2 { 1.0, 1.0 } };
  ofParameter<float> field2MultiplierParameter { "Field2Multiplier", 0.005, 0.0, 0.05 };
  ParamController<float> field2MultiplierController { field2MultiplierParameter };
//  ofParameter<glm::vec2> field2BiasParameter { "Field2Bias", glm::vec2 { -0.5, -0.5 }, glm::vec2 { -1.0, -1.0 }, glm::vec2 { 1.0, 1.0 } };
  ofParameter<glm::vec2> field2BiasParameter { "Field2Bias", glm::vec2 { 0.0, 0.0 }, glm::vec2 { -1.0, -1.0 }, glm::vec2 { 1.0, 1.0 } };
  
  ofParameter<glm::vec2> gridSizeParameter { "GridSize", glm::vec2 { 8.0, 8.0 }, glm::vec2 { 2.0, 2.0 }, glm::vec2 { 128.0, 128.0 } };
  ofParameter<int> strategyParameter { "Strategy", 0, 0, 9 }; // 0: Off; 1: Cell-quantized; 2: Per-cell random offset; 3: Boundary teleport; 4. Per-cell rotation/reflection; 5. Multi-res grid snap; 6. Voronoi partition teleport; 7. Border kill-band; 8. Dual-sample ghosting on border cross; 9. Piecewise mirroring/folding
  ofParameter<float> jumpAmountParameter { "JumpAmount2", 0.5, 0.0, 1.0 };
  ParamController<float> jumpAmountController { jumpAmountParameter }; // only for strategy 2
  ofParameter<float> borderWidthParameter { "BorderWidth7", 0.05, 0.0, 0.49 };
  ParamController<float> borderWidthController { borderWidthParameter }; // only for strategy 7
  ofParameter<int> gridLevelsParameter { "GridLevels5", 1, 1, 16 }; // only for strategy 5
  ofParameter<float> ghostBlendParameter { "GhostBlend8", 0.5, 0.0, 1.0 };
  ParamController<float> ghostBlendController { ghostBlendParameter }; // only for strategy 8
  ofParameter<glm::vec2> foldPeriodParameter { "FoldPeriod9", glm::vec2 { 8.0f, 8.0f }, glm::vec2 { 0.0, 0.0 }, glm::vec2 { 64.0f, 64.0f } }; // only for strategy 9
  ofParameter<float> agencyFactorParameter { "AgencyFactor", 1.0, 0.0, 1.0 };

  SmearShader smearShader;
  
  ofTexture field1Tex, field2Tex;
};


} // ofxMarkSynth
