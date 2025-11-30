//
//  Oklab.h
//  ofxMarkSynth
//
//  Oklab perceptual color space conversions
//  Based on Björn Ottosson's Oklab: https://bottosson.github.io/posts/oklab/
//

#pragma once

#include "ofMain.h"
#include <algorithm>
#include <cmath>

namespace ofxMarkSynth {

struct Oklab {
    float L;  // Lightness [0, 1]
    float a;  // Green-red axis (roughly [-0.4, 0.4])
    float b;  // Blue-yellow axis (roughly [-0.4, 0.4])
};

// Convert linear sRGB [0,1] to Oklab
inline Oklab linearRgbToOklab(float r, float g, float b) {
    float l = 0.4122214708f * r + 0.5363325363f * g + 0.0514459929f * b;
    float m = 0.2119034982f * r + 0.6806995451f * g + 0.1073969566f * b;
    float s = 0.0883024619f * r + 0.2817188376f * g + 0.6299787005f * b;

    float l_ = std::cbrt(l);
    float m_ = std::cbrt(m);
    float s_ = std::cbrt(s);

    return {
        0.2104542553f * l_ + 0.7936177850f * m_ - 0.0040720468f * s_,
        1.9779984951f * l_ - 2.4285922050f * m_ + 0.4505937099f * s_,
        0.0259040371f * l_ + 0.7827717662f * m_ - 0.8086757660f * s_
    };
}

// Convert Oklab to linear sRGB [0,1]
inline void oklabToLinearRgb(const Oklab& lab, float& r, float& g, float& b) {
    float l_ = lab.L + 0.3963377774f * lab.a + 0.2158037573f * lab.b;
    float m_ = lab.L - 0.1055613458f * lab.a - 0.0638541728f * lab.b;
    float s_ = lab.L - 0.0894841775f * lab.a - 1.2914855480f * lab.b;

    float l = l_ * l_ * l_;
    float m = m_ * m_ * m_;
    float s = s_ * s_ * s_;

    r =  4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s;
    g = -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s;
    b = -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s;
}

// sRGB gamma: linear to sRGB
inline float linearToSrgb(float x) {
    if (x <= 0.0031308f) return 12.92f * x;
    return 1.055f * std::pow(x, 1.0f / 2.4f) - 0.055f;
}

// sRGB gamma: sRGB to linear
inline float srgbToLinear(float x) {
    if (x <= 0.04045f) return x / 12.92f;
    return std::pow((x + 0.055f) / 1.055f, 2.4f);
}

// Convert ofFloatColor (sRGB) to Oklab
inline Oklab rgbToOklab(const ofFloatColor& c) {
    float r = srgbToLinear(c.r);
    float g = srgbToLinear(c.g);
    float b = srgbToLinear(c.b);
    return linearRgbToOklab(r, g, b);
}

// Convert Oklab to ofFloatColor (sRGB), with specified alpha
inline ofFloatColor oklabToRgb(const Oklab& lab, float alpha = 1.0f) {
    float r, g, b;
    oklabToLinearRgb(lab, r, g, b);
    
    // Clamp to valid range before gamma correction
    r = std::clamp(r, 0.0f, 1.0f);
    g = std::clamp(g, 0.0f, 1.0f);
    b = std::clamp(b, 0.0f, 1.0f);
    
    return ofFloatColor {
        linearToSrgb(r),
        linearToSrgb(g),
        linearToSrgb(b),
        alpha
    };
}

} // namespace ofxMarkSynth
