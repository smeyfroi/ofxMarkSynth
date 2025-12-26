//
//  IntentController.cpp
//  ofxMarkSynth
//

#include "util/IntentController.hpp"
#include "ofMain.h"

namespace ofxMarkSynth {

void IntentController::setPresets(const std::vector<IntentPtr>& presets) {
    if (presets.size() > 7) {
        ofLogWarning("IntentController") << "Received " << presets.size() << " intents, limiting to 7";
    }

    activations.clear();
    activationParameters.clear();

    size_t count = std::min(presets.size(), size_t(7));
    for (size_t i = 0; i < count; ++i) {
        activations.emplace_back(presets[i]);
        auto p = std::make_shared<ofParameter<float>>(presets[i]->getName() + " Activation", 0.0f, 0.0f, 1.0f);
        activationParameters.push_back(p);
    }

    rebuildParameterGroup();
    ofLogNotice("IntentController") << "Set " << count << " intent presets";
}

void IntentController::setStrength(float value) {
    strengthParameter.set(ofClamp(value, 0.0f, 1.0f));
}

void IntentController::setActivation(size_t index, float value) {
    if (index >= activations.size()) {
        ofLogWarning("IntentController") << "setActivation: index " << index
                                         << " out of range (have " << activations.size() << " intents)";
        return;
    }
    float clamped = ofClamp(value, 0.0f, 1.0f);
    activations[index].activation = clamped;
    activations[index].targetActivation = clamped;
    if (index < activationParameters.size()) {
        activationParameters[index]->set(clamped);
    }
}

void IntentController::update() {
    updateActivations();
    computeActiveIntent();
    updateInfoLabels();
}

float IntentController::getEffectiveStrength() const {
    float totalActivation = 0.0f;
    for (const auto& ia : activations) {
        totalActivation += ia.activation;
    }
    return strengthParameter.get() * std::min(1.0f, totalActivation);
}

void IntentController::rebuildParameterGroup() {
    parameters.clear();
    parameters.setName("Intent");

    for (auto& p : activationParameters) {
        parameters.add(*p);
    }

    // Keep master intent strength at the end so it appears rightmost in the GUI.
    parameters.add(strengthParameter);
}

void IntentController::updateActivations() {
    float dt = ofGetLastFrameTime();
    for (size_t i = 0; i < activations.size(); ++i) {
        auto& ia = activations[i];
        float target = activationParameters[i]->get();
        ia.targetActivation = target;
        float speed = ia.transitionSpeed;
        float alpha = 1.0f - expf(-dt * std::max(0.001f, speed) * 4.0f);
        ia.activation = ofLerp(ia.activation, ia.targetActivation, alpha);
    }
}

void IntentController::computeActiveIntent() {
    static std::vector<std::pair<IntentPtr, float>> weighted;
    if (weighted.size() != activations.size()) {
        weighted.clear();
        weighted.resize(activations.size());
    }
    for (size_t i = 0; i < activations.size(); ++i) {
        auto& ia = activations[i];
        weighted[i].first = ia.intentPtr;
        weighted[i].second = ia.activation;
    }
    activeIntent.setWeightedBlend(weighted);
}

void IntentController::updateInfoLabels() {
    infoLabel1 = "E" + ofToString(activeIntent.getEnergy(), 2) +
                 " D" + ofToString(activeIntent.getDensity(), 2) +
                 " C" + ofToString(activeIntent.getChaos(), 2);
    infoLabel2 = "S" + ofToString(activeIntent.getStructure(), 2) +
                 " G" + ofToString(activeIntent.getGranularity(), 2);
}

} // namespace ofxMarkSynth
