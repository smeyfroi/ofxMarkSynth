//
//  LayerController.cpp
//  ofxMarkSynth
//

#include "util/LayerController.hpp"
#include "ofMain.h"

namespace ofxMarkSynth {

DrawingLayerPtr LayerController::addLayer(const std::string& name, glm::vec2 size,
                                          GLint internalFormat, int wrap,
                                          bool clearOnUpdate, ofBlendMode blendMode,
                                          bool useStencil, int numSamples,
                                          bool isDrawn, bool isOverlay,
                                          const std::string& description) {
    auto fboPtr = std::make_shared<PingPongFbo>();
    fboPtr->allocate(size, internalFormat, wrap, useStencil, numSamples);
    fboPtr->clearFloat(ofFloatColor(0, 0, 0, 0));
    
    auto layerPtr = std::make_shared<DrawingLayer>(name, fboPtr, clearOnUpdate,
                                                    blendMode, isDrawn, isOverlay, description);
    layers.insert({ name, layerPtr });
    return layerPtr;
}

void LayerController::setInitialAlpha(const std::string& name, float alpha) {
    initialAlphas[name] = alpha;
}

void LayerController::setInitialPaused(const std::string& name, bool paused) {
    initialPaused[name] = paused;
}

void LayerController::buildAlphaParameters() {
    alphaParameters.clear();
    alphaParamPtrs.clear();
    
    alphaParameters.setName("Layers");
    for (const auto& [name, layerPtr] : layers) {
        if (!layerPtr->isDrawn) continue;
        
        float alpha = 1.0f;
        auto it = initialAlphas.find(layerPtr->name);
        if (it != initialAlphas.end()) {
            alpha = it->second;
        }
        
        auto param = std::make_shared<ofParameter<float>>(layerPtr->name, alpha, 0.0f, 1.0f);
        alphaParamPtrs.push_back(param);
        alphaParameters.add(*param);
    }
}

void LayerController::buildPauseParameters() {
    pauseParameters.clear();
    pauseParamPtrs.clear();
    
    pauseParameters.setName("Layer Pauses");
    for (const auto& [name, layerPtr] : layers) {
        if (!layerPtr->isDrawn) continue;
        
        bool paused = false;
        auto it = initialPaused.find(layerPtr->name);
        if (it != initialPaused.end()) {
            paused = it->second;
        }
        
        auto param = std::make_shared<ofParameter<bool>>(layerPtr->name + " Paused", paused);
        pauseParamPtrs.push_back(param);
        pauseParameters.add(*param);
        
        layerPtr->pauseState = paused ? DrawingLayer::PauseState::PAUSED
                                      : DrawingLayer::PauseState::ACTIVE;
    }
}

void LayerController::updatePauseStates() {
    // Build lookup: base layer name -> pause parameter
    std::unordered_map<std::string, ofParameter<bool>*> pauseByName;
    for (auto& p : pauseParamPtrs) {
        std::string baseName = p->getName();
        auto pos = baseName.rfind(" Paused");
        if (pos != std::string::npos) {
            baseName = baseName.substr(0, pos);
        }
        pauseByName[baseName] = p.get();
    }
    
    for (auto& [name, layerPtr] : layers) {
        if (!layerPtr->isDrawn) continue;
        
        bool targetPaused = false;
        if (auto it = pauseByName.find(name); it != pauseByName.end()) {
            targetPaused = it->second->get();
        }
        
        layerPtr->pauseState = targetPaused ? DrawingLayer::PauseState::PAUSED
                                            : DrawingLayer::PauseState::ACTIVE;
    }
}

void LayerController::clearActiveLayers(const ofFloatColor& clearColor) {
    for (auto& [name, layerPtr] : layers) {
        if (layerPtr->clearOnUpdate && layerPtr->pauseState != DrawingLayer::PauseState::PAUSED) {
            layerPtr->fboPtr->getSource().begin();
            ofClear(clearColor);
            layerPtr->fboPtr->getSource().end();
        }
    }
}

void LayerController::clear() {
    layers.clear();
    initialAlphas.clear();
    initialPaused.clear();
    alphaParamPtrs.clear();
    alphaParameters.clear();
    pauseParamPtrs.clear();
    pauseParameters.clear();
}

bool LayerController::togglePause(int index) {
    if (index < 0 || index >= static_cast<int>(pauseParamPtrs.size())) {
        return false;
    }
    
    auto& p = pauseParamPtrs[index];
    bool newState = !p->get();
    p->set(newState);
    return newState;
}

} // namespace ofxMarkSynth
