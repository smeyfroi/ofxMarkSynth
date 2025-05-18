//
//  DividedAreaMod.hpp
//  example_audio_clusters
//
//  Created by Steve Meyfroidt on 07/05/2025.
//

#pragma once

#include "ofxGui.h"
#include "Mod.hpp"
#include "ofxDividedArea.h"


namespace ofxMarkSynth {


// SINK_FBO is used to draw unconstrained lines
// SINK_FBO_1 is used to draw constrained lines
class DividedAreaMod : public Mod {

public:
  DividedAreaMod(const std::string& name, const ModConfig&& config);
  void update() override;
  void receive(int sinkId, const glm::vec2& point) override;

  static constexpr int SINK_MAJOR_ANCHORS = 1;
  static constexpr int SINK_MINOR_ANCHORS = 10;
  // TODO: an alternative strategy would add constrained lines at some angle through an anchor
  // TODO: an alternative strategy would add constrained lines centred on one anchor through the rest

  // SINK_FBO for drawing constrained lines
  // SINK_FBO_2 for drawing unconstrained lines
  
protected:
  void initParameters() override;

private:
  std::vector<glm::vec2> newMajorAnchors;
  std::vector<glm::vec2> newMinorAnchors;
  DividedArea dividedArea;
};


} // ofxMarkSynth
