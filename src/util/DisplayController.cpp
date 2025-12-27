//
//  DisplayController.cpp
//  ofxMarkSynth
//

#include "DisplayController.hpp"

namespace ofxMarkSynth {

void DisplayController::buildParameterGroup() {
    parameters.clear();
    parameters.setName("Display");
    parameters.add(toneMapType);
    parameters.add(exposure);
    parameters.add(gamma);
    parameters.add(whitePoint);
    parameters.add(contrast);
    parameters.add(saturation);
    parameters.add(brightness);
    parameters.add(hueShift);
    parameters.add(sideExposure);
}

DisplayController::Settings DisplayController::getSettings() const {
    return Settings {
        .toneMapType = toneMapType.get(),
        .exposure = exposure.get(),
        .gamma = gamma.get(),
        .whitePoint = whitePoint.get(),
        .contrast = contrast.get(),
        .saturation = saturation.get(),
        .brightness = brightness.get(),
        .hueShift = hueShift.get()
    };
}

DisplayController::Settings DisplayController::getSidePanelSettings() const {
    return Settings {
        .toneMapType = toneMapType.get(),
        .exposure = sideExposure.get(),  // Use side exposure for side panels
        .gamma = gamma.get(),
        .whitePoint = whitePoint.get(),
        .contrast = contrast.get(),
        .saturation = saturation.get(),
        .brightness = brightness.get(),
        .hueShift = hueShift.get()
    };
}

} // namespace ofxMarkSynth
