//
//  PixelSnapshotMod.hpp
//  example_collage
//
//  Created by Steve Meyfroidt on 18/05/2025.
//

#pragma once

#include "core/Mod.hpp"
#include "core/ParamController.h"



namespace ofxMarkSynth {



class PixelSnapshotMod : public Mod {

public:
  PixelSnapshotMod(std::shared_ptr<Synth> synthPtr, const std::string& name, ModConfig config);
  float getAgency() const override;
  void update() override;
  void draw() override;
  bool keyPressed(int key) override;
  void applyIntent(const Intent& intent, float strength) override;

  static constexpr int SOURCE_SNAPSHOT_TEXTURE = 11;

protected:
  void initParameters() override;

private:
  float updateCount;
  ofParameter<float> snapshotsPerUpdateParameter { "SnapshotsPerUpdate", 1.0/30.0, 0.0, 1.0 };
  ofParameter<float> sizeParameter { "Size", 1024, 128, 8096 }; // must be smaller than the source layer
  ParamController<float> sizeController { sizeParameter };
  ofParameter<float> agencyFactorParameter { "AgencyFactor", 1.0, 0.0, 1.0 };

  ofFbo snapshotFbo; // Scratchpad FBO for GPU-based cropping operation
  const ofTexture& createSnapshot(const ofFbo& sourceFbo);

  bool visible = false;
};



} // ofxMarkSynth
