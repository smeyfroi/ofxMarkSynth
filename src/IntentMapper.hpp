#pragma once

#include "Intent.hpp"
#include "ParamController.h"
#include <string>
#include <cmath>
#include <cstdio>

namespace ofxMarkSynth {

// Represents a mapping from Intent dimension(s) to a value
class Mapping {
public:
    Mapping(float value, std::string label)
        : value_(value), label_(std::move(label)) {}

    // Combine dimensions via multiplication
    Mapping operator*(const Mapping& other) const {
        return Mapping(value_ * other.value_, label_ + "*" + other.label_);
    }

    // Inverse (1 - value)
    Mapping inv() const {
        return Mapping(1.0f - value_, "1-" + label_);
    }

    // --- Mapping functions that apply to controller ---

    // Linear map using controller's min/max range
    void lin(ParamController<float>& ctrl, float strength) const {
        float min = ctrl.getManualMin();
        float max = ctrl.getManualMax();
        float result = ofLerp(min, max, value_);
        ctrl.updateIntent(result, strength, label_ + " -> lin");
    }

    // Linear map with custom range
    void lin(ParamController<float>& ctrl, float strength, float min, float max) const {
        float result = ofLerp(min, max, value_);
        ctrl.updateIntent(result, strength,
            label_ + " -> lin [" + fmt(min) + ", " + fmt(max) + "]");
    }

    // Exponential map using controller's min/max range
    void exp(ParamController<float>& ctrl, float strength, float exponent = 2.0f) const {
        float min = ctrl.getManualMin();
        float max = ctrl.getManualMax();
        float curved = std::pow(ofClamp(value_, 0.0f, 1.0f), exponent);
        float result = ofLerp(min, max, curved);
        ctrl.updateIntent(result, strength,
            label_ + " -> exp(" + fmt(exponent) + ")");
    }

    // Exponential map with custom range
    void exp(ParamController<float>& ctrl, float strength,
             float min, float max, float exponent) const {
        float curved = std::pow(ofClamp(value_, 0.0f, 1.0f), exponent);
        float result = ofLerp(min, max, curved);
        ctrl.updateIntent(result, strength,
            label_ + " -> exp(" + fmt(exponent) + ") [" + fmt(min) + ", " + fmt(max) + "]");
    }

    // Get raw value for manual compositions (e.g., color building)
    float get() const { return value_; }

    // Get label for manual description building
    const std::string& getLabel() const { return label_; }

private:
    float value_;
    std::string label_;

    static std::string fmt(float v) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%.2g", v);
        return buf;
    }
};

// Main entry point - holds reference to Intent and provides dimension accessors
class IntentMap {
public:
    explicit IntentMap(const Intent& intent) : intent_(intent) {}

    // Dimension accessors - return Mapping objects for chaining
    Mapping E() const { return Mapping(intent_.getEnergy(), "E"); }
    Mapping D() const { return Mapping(intent_.getDensity(), "D"); }
    Mapping S() const { return Mapping(intent_.getStructure(), "S"); }
    Mapping C() const { return Mapping(intent_.getChaos(), "C"); }
    Mapping G() const { return Mapping(intent_.getGranularity(), "G"); }

    // Access raw Intent for complex operations (e.g., energyToColor)
    const Intent& intent() const { return intent_; }

private:
    const Intent& intent_;
};

} // namespace ofxMarkSynth
