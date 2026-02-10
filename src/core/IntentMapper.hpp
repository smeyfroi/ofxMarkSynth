#pragma once

#include "core/Intent.hpp"
#include "core/ParamController.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <utility>

namespace ofxMarkSynth {

namespace IntentMapperDefaults {
constexpr float AROUND_MANUAL_UP_FRACTION = 0.50f;
constexpr float AROUND_MANUAL_DOWN_FRACTION = 0.70f;
constexpr float AROUND_MANUAL_EXPONENT = 2.0f;
}

// Represents a mapping from Intent dimension(s) to a value
class Mapping {
public:
    // Tagged range wrapper (avoids overload ambiguity and float-soup).
    struct WithRange {
        float min;
        float max;
    };

    // Tagged fraction wrapper (avoids float-soup).
    struct WithFractions {
        float up;
        float down;
    };

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

    // Linear map with tagged custom range
    void lin(ParamController<float>& ctrl, float strength, WithRange range) const {
        float result = ofLerp(range.min, range.max, value_);
        ctrl.updateIntent(result,
                          strength,
                          label_ + " -> lin [" + fmt(range.min) + ", " + fmt(range.max) + "]");
    }

    // Exponential map using controller's min/max range
    void exp(ParamController<float>& ctrl, float strength, float exponent = 2.0f) const {
        float min = ctrl.getManualMin();
        float max = ctrl.getManualMax();
        float curved = std::pow(ofClamp(value_, 0.0f, 1.0f), exponent);
        float result = ofLerp(min, max, curved);
        ctrl.updateIntent(result,
                          strength,
                          label_ + " -> exp(" + fmt(exponent) + ")");
    }

    // Exponential map with tagged custom range
    void exp(ParamController<float>& ctrl,
             float strength,
             WithRange range,
             float exponent = 2.0f) const {
        float curved = std::pow(ofClamp(value_, 0.0f, 1.0f), exponent);
        float result = ofLerp(range.min, range.max, curved);
        ctrl.updateIntent(result,
                          strength,
                          label_ + " -> exp(" + fmt(exponent) + ") [" + fmt(range.min) + ", "
                              + fmt(range.max) + "]");
    }

    // Linear mapping around the current manual value, with a bounded band.
    // fractions are expressed as fractions of (max-min).
    void linAround(ParamController<float>& ctrl,
                  float strength,
                  WithFractions fractions = WithFractions{IntentMapperDefaults::AROUND_MANUAL_UP_FRACTION,
                                                         IntentMapperDefaults::AROUND_MANUAL_DOWN_FRACTION}) const {
        linAround(ctrl, strength, WithRange{ctrl.getManualMin(), ctrl.getManualMax()}, fractions);
    }

    void linAround(ParamController<float>& ctrl,
                  float strength,
                  WithRange range,
                  WithFractions fractions = WithFractions{IntentMapperDefaults::AROUND_MANUAL_UP_FRACTION,
                                                         IntentMapperDefaults::AROUND_MANUAL_DOWN_FRACTION}) const {
        float result = aroundManual(value_,
                                    ctrl.getManualValue(),
                                    range.min,
                                    range.max,
                                    fractions,
                                    1.0f);
        ctrl.updateIntent(result,
                          strength,
                          label_ + " -> linAround up=" + fmt(fractions.up) + " down=" + fmt(fractions.down)
                              + " [" + fmt(range.min) + ", " + fmt(range.max) + "]");
    }

    void expAround(ParamController<float>& ctrl,
                  float strength,
                  float exponent = IntentMapperDefaults::AROUND_MANUAL_EXPONENT,
                  WithFractions fractions = WithFractions{IntentMapperDefaults::AROUND_MANUAL_UP_FRACTION,
                                                         IntentMapperDefaults::AROUND_MANUAL_DOWN_FRACTION}) const {
        expAround(ctrl,
                 strength,
                 WithRange{ctrl.getManualMin(), ctrl.getManualMax()},
                 exponent,
                 fractions);
    }

    void expAround(ParamController<float>& ctrl,
                  float strength,
                  WithRange range,
                  float exponent = IntentMapperDefaults::AROUND_MANUAL_EXPONENT,
                  WithFractions fractions = WithFractions{IntentMapperDefaults::AROUND_MANUAL_UP_FRACTION,
                                                         IntentMapperDefaults::AROUND_MANUAL_DOWN_FRACTION}) const {
        float result = aroundManual(value_,
                                    ctrl.getManualValue(),
                                    range.min,
                                    range.max,
                                    fractions,
                                    exponent);
        ctrl.updateIntent(result,
                          strength,
                          label_ + " -> expAround(" + fmt(exponent) + ") up=" + fmt(fractions.up)
                              + " down=" + fmt(fractions.down) + " [" + fmt(range.min) + ", "
                              + fmt(range.max) + "]");
    }

    // Get raw value for manual compositions (e.g., color building)
    float get() const { return value_; }

    // Get label for manual description building
    const std::string& getLabel() const { return label_; }

private:
    float value_;
    std::string label_;

    static float aroundManual(float value01,
                              float manualValue,
                              float min,
                              float max,
                              WithFractions fractions,
                              float exponent) {
        float clampedMin = std::min(min, max);
        float clampedMax = std::max(min, max);
        float range = clampedMax - clampedMin;
        if (!(range > 1e-12f)) {
            return std::clamp(manualValue, clampedMin, clampedMax);
        }

        float t = std::clamp(value01, 0.0f, 1.0f);
        float signedDist = (t - 0.5f) * 2.0f; // [-1, 1]
        float dist = std::abs(signedDist);
        float curved = (exponent == 1.0f) ? dist : std::pow(dist, exponent);

        float manual = std::clamp(manualValue, clampedMin, clampedMax);
        float up = std::max(0.0f, fractions.up) * range;
        float down = std::max(0.0f, fractions.down) * range;

        float result = manual;
        if (signedDist >= 0.0f) {
            result += curved * up;
        } else {
            result -= curved * down;
        }
        return std::clamp(result, clampedMin, clampedMax);
    }

    static std::string fmt(float v) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%.2g", v);
        return buf;
    }
};

// Main entry point - holds reference to Intent and provides dimension accessors
class IntentMap {
public:
    explicit IntentMap(const Intent& intent)
        : intent_(intent) {}

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
