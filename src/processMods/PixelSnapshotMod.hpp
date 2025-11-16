//
//  PixelSnapshotMod.hpp
//  example_collage
//
//  Created by Steve Meyfroidt on 18/05/2025.
//

#pragma once

#include "Mod.hpp"
#include "ParamController.h"



namespace ofxMarkSynth {



class PixelSnapshotMod : public Mod {

public:
  PixelSnapshotMod(Synth* synthPtr, const std::string& name, ModConfig config);
  void update() override;
  void draw() override;
  bool keyPressed(int key) override;
  void receive(int sinkId, const ofFbo& value) override;
  void applyIntent(const Intent& intent, float strength) override;

  static constexpr int SINK_SNAPSHOT_SOURCE = 10;
  static constexpr int SOURCE_SNAPSHOT = 11;

protected:
  void initParameters() override;

private:
  float updateCount;
  ofParameter<float> snapshotsPerUpdateParameter { "SnapshotsPerUpdate", 1.0/30.0, 0.0, 1.0 };
  ofParameter<float> sizeParameter { "Size", 1024, 128, 8096 }; // must be smaller than the source FBO
  ParamController<float> sizeController { sizeParameter };

  ofFbo sourceFbo;
  ofFbo snapshotFbo;
  const ofFbo createSnapshot(const ofFbo& fbo);

  bool visible = false;
};



} // ofxMarkSynth
