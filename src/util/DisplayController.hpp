//
//  DisplayController.hpp
//  ofxMarkSynth
//
//  Manages display/tonemap parameters for final output rendering.
//

#pragma once

#include "ofParameter.h"

namespace ofxMarkSynth {

/// Manages display and tonemapping parameters.
class DisplayController {
public:
    DisplayController() = default;

    /// Build the parameter group (call after construction)
    void buildParameterGroup();

    /// Parameter group for GUI
    ofParameterGroup& getParameterGroup() { return parameters; }
    const ofParameterGroup& getParameterGroup() const { return parameters; }

    /// Individual parameter accessors for GUI sliders
    ofParameter<int>& getToneMapType() { return toneMapType; }
    ofParameter<float>& getExposure() { return exposure; }
    ofParameter<float>& getGamma() { return gamma; }
    ofParameter<float>& getWhitePoint() { return whitePoint; }
    ofParameter<float>& getContrast() { return contrast; }
    ofParameter<float>& getSaturation() { return saturation; }
    ofParameter<float>& getBrightness() { return brightness; }
    ofParameter<float>& getHueShift() { return hueShift; }
    ofParameter<float>& getSideExposure() { return sideExposure; }

    const ofParameter<int>& getToneMapType() const { return toneMapType; }
    const ofParameter<float>& getExposure() const { return exposure; }
    const ofParameter<float>& getGamma() const { return gamma; }
    const ofParameter<float>& getWhitePoint() const { return whitePoint; }
    const ofParameter<float>& getContrast() const { return contrast; }
    const ofParameter<float>& getSaturation() const { return saturation; }
    const ofParameter<float>& getBrightness() const { return brightness; }
    const ofParameter<float>& getHueShift() const { return hueShift; }
    const ofParameter<float>& getSideExposure() const { return sideExposure; }

    /// Settings struct for passing to shader
    struct Settings {
        int toneMapType;
        float exposure;
        float gamma;
        float whitePoint;
        float contrast;
        float saturation;
        float brightness;
        float hueShift;
    };

    /// Get current settings for main panel (uses exposure)
    Settings getSettings() const;

    /// Get settings for side panels (uses sideExposure instead of exposure)
    Settings getSidePanelSettings() const;

private:
    ofParameterGroup parameters;

    ofParameter<int> toneMapType { "Tone map", 3, 0, 5 };
    ofParameter<float> exposure { "Exposure", 1.0, 0.0, 4.0 };
    ofParameter<float> gamma { "Gamma", 2.2, 0.1, 5.0 };
    ofParameter<float> whitePoint { "White Pt", 11.2, 1.0, 20.0 };
    ofParameter<float> contrast { "Contrast", 1.03, 0.9, 1.1 };
    ofParameter<float> saturation { "Saturation", 1.0, 0.0, 2.0 };
    ofParameter<float> brightness { "Brightness", 0.0, -0.1, 0.1 };
    ofParameter<float> hueShift { "Hue Shift", 0.0, -1.0, 1.0 };
    ofParameter<float> sideExposure { "Side Exp", 0.6, 0.0, 4.0 };
};

} // namespace ofxMarkSynth
