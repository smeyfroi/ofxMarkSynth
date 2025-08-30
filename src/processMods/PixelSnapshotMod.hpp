//
//  PixelSnapshotMod.hpp
//  example_collage
//
//  Created by Steve Meyfroidt on 18/05/2025.
//

#pragma once

#include "Mod.hpp"

namespace ofxMarkSynth {


class PixelSnapshotMod : public Mod {

public:
  PixelSnapshotMod(const std::string& name, const ModConfig&& config);
  void update() override;
  void draw() override;
  bool keyPressed(int key) override;

  static constexpr int SOURCE_PIXELS = 1;

protected:
  void initParameters() override;

private:
  float updateCount;
  ofParameter<float> snapshotsPerUpdateParameter { "SnapshotsPerUpdate", 1.0/30.0, 0.0, 1.0 };
  ofParameter<int> sizeParameter { "Size", 1024, 0, 10240 };
  
  ofFloatPixels pixels;
  const ofFloatPixels createPixels(const FboPtr& fboPtr);

  bool visible = false;
};


} // ofxMarkSynth

