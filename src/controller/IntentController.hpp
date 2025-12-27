//
//  IntentController.hpp
//  ofxMarkSynth
//

#pragma once

#include "core/Intent.hpp"
#include "ofParameter.h"

namespace ofxMarkSynth {

/// Manages intent activations, smoothing, and weighted blending.
/// Computes the active intent from multiple preset activations.
class IntentController {
public:
    IntentController() = default;

    /// Set intent presets (max 7). Clears existing activations.
    void setPresets(const std::vector<IntentPtr>& presets);

    /// Set master intent strength (0-1)
    void setStrength(float value);

    /// Set activation for a specific intent index (0-1)
    void setActivation(size_t index, float value);

    /// Update activations (smoothing) and compute weighted blend. Call each frame.
    void update();

    // Accessors
    const Intent& getActiveIntent() const { return activeIntent; }
    float getStrength() const { return strengthParameter.get(); }
    float getEffectiveStrength() const;  // strength * clamped total activation
    size_t getCount() const { return activations.size(); }
    
    // For GUI direct access
    const IntentActivations& getActivations() const { return activations; }
    const std::vector<std::shared_ptr<ofParameter<float>>>& getActivationParameters() const { return activationParameters; }
    std::vector<std::shared_ptr<ofParameter<float>>>& getActivationParameters() { return activationParameters; }

    // For GUI/serialization
    ofParameterGroup& getParameterGroup() { return parameters; }
    const std::string& getInfoLabel1() const { return infoLabel1; }
    const std::string& getInfoLabel2() const { return infoLabel2; }

private:
    void rebuildParameterGroup();
    void updateActivations();
    void computeActiveIntent();
    void updateInfoLabels();

    IntentActivations activations;
    Intent activeIntent { "Active", 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    ofParameterGroup parameters;
    ofParameter<float> strengthParameter { "Intent Strength", 0.0f, 0.0f, 1.0f };
    std::vector<std::shared_ptr<ofParameter<float>>> activationParameters;
    std::string infoLabel1;
    std::string infoLabel2;
};

} // namespace ofxMarkSynth
