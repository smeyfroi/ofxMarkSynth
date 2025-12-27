//
//  LayerController.hpp
//  ofxMarkSynth
//

#pragma once

#include "core/Mod.hpp"
#include "ofParameter.h"
#include <map>
#include <unordered_map>

namespace ofxMarkSynth {

using DrawingLayerPtrMap = std::map<std::string, DrawingLayerPtr>;

/// Manages drawing layers - their creation, alpha/pause parameters, and state updates.
class LayerController {
public:
    LayerController() = default;

    /// Create and add a new drawing layer
    DrawingLayerPtr addLayer(const std::string& name, glm::vec2 size,
                             GLint internalFormat, int wrap,
                             bool clearOnUpdate, ofBlendMode blendMode,
                             bool useStencil, int numSamples,
                             bool isDrawn = true, bool isOverlay = false,
                             const std::string& description = "");

    /// Set initial alpha for a layer (call before buildAlphaParameters)
    void setInitialAlpha(const std::string& name, float alpha);

    /// Set initial paused state for a layer (call before buildPauseParameters)
    void setInitialPaused(const std::string& name, bool paused);

    /// Build alpha parameter group from current layers
    void buildAlphaParameters();

    /// Build pause parameter group from current layers
    void buildPauseParameters();

    /// Update layer pause states from parameters (call each frame)
    void updatePauseStates();

    /// Clear FBOs for active (non-paused) layers with clearOnUpdate flag
    void clearActiveLayers(const ofFloatColor& clearColor);

    /// Clear all layers and parameters (for config unload)
    void clear();

    // Accessors
    const DrawingLayerPtrMap& getLayers() const { return layers; }
    DrawingLayerPtrMap& getLayers() { return layers; }
    size_t getCount() const { return layers.size(); }

    // For GUI/Synth
    ofParameterGroup& getAlphaParameterGroup() { return alphaParameters; }
    ofParameterGroup& getPauseParameterGroup() { return pauseParameters; }
    const std::vector<std::shared_ptr<ofParameter<bool>>>& getPauseParamPtrs() const { return pauseParamPtrs; }

    /// Toggle pause state for layer at index. Returns new paused state.
    bool togglePause(int index);

private:
    DrawingLayerPtrMap layers;
    std::unordered_map<std::string, float> initialAlphas;
    std::unordered_map<std::string, bool> initialPaused;

    ofParameterGroup alphaParameters;
    std::vector<std::shared_ptr<ofParameter<float>>> alphaParamPtrs;
    ofParameterGroup pauseParameters;
    std::vector<std::shared_ptr<ofParameter<bool>>> pauseParamPtrs;
};

} // namespace ofxMarkSynth
