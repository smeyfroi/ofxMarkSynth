//
//  NodeRenderUtil.hpp
//  fingerprint1
//
//  Created by Steve Meyfroidt on 12/11/2025.
//

#pragma once

#include "ofParameter.h"
#include "Mod.hpp"

struct ImFont;  // Forward declaration



namespace ofxMarkSynth {
namespace NodeRenderUtil {

// Set the monospace font to use in tooltips (call from Gui setup)
void setMonoFont(ImFont* font);



void drawVerticalSliders(ofParameterGroup& paramGroup);
void drawVerticalSliders(ofParameterGroup& paramGroup, const std::vector<std::shared_ptr<ofParameter<bool>>>& toggleParams);
void addParameter(const ModPtr& modPtr, ofParameter<int>& parameter);
void addParameter(const ModPtr& modPtr, ofParameter<float>& parameter);
void addParameter(const ModPtr& modPtr, ofParameter<bool>& parameter);
void addParameter(const ModPtr& modPtr, ofParameter<std::string>& parameter);
void addParameter(const ModPtr& modPtr, ofParameter<ofFloatColor>& parameter);
void addParameter(const ModPtr& modPtr, ofParameter<glm::vec2>& parameter);
void addParameter(const ModPtr& modPtr, ofAbstractParameter& parameterPtr);
void addParameterGroup(const ModPtr& modPtr, ofParameterGroup& paramGroup);
void addContributionWeights(const ModPtr& modPtr, const std::string& paramName);



} // namespace NodeRenderUtil
} // namespace ofxMarkSynth
