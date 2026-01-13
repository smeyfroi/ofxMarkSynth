//
//  CompositeRenderer.cpp
//  ofxMarkSynth
//

#include "rendering/CompositeRenderer.hpp"
#include "rendering/RenderingConstants.h"

#include "ofGraphics.h"
#include "ofAppRunner.h"

namespace ofxMarkSynth {

namespace {

glm::vec2 randomCentralRectOrigin(glm::vec2 rectSize, glm::vec2 bounds) {
    float x = ofRandom(bounds.x * PANEL_ORIGIN_MIN_FRAC, bounds.x * PANEL_ORIGIN_MAX_FRAC - rectSize.x);
    float y = ofRandom(bounds.y * PANEL_ORIGIN_MIN_FRAC, bounds.y * PANEL_ORIGIN_MAX_FRAC - rectSize.y);
    return { x, y };
}

float easeInCubic(float x) {
    return x * x * x;
}

} // anonymous namespace

void CompositeRenderer::allocate(glm::vec2 compositeSize, float windowWidth, float windowHeight, float panelGapPx) {
    size = compositeSize;
    
    compositeFbo.allocate(size.x, size.y, GL_RGB16F);
    scale = std::min(windowWidth / compositeFbo.getWidth(), windowHeight / compositeFbo.getHeight());
    
    // Side panels
    panelWidth = (windowWidth - compositeFbo.getWidth() * scale) / 2.0f - panelGapPx;
    if (panelWidth > 0.0f) {
        panelHeight = windowHeight;
        leftPanel.fbo.allocate(panelWidth, panelHeight, GL_RGB16F);
        rightPanel.fbo.allocate(panelWidth, panelHeight, GL_RGB16F);
        leftPanel.timeoutSecs = LEFT_PANEL_TIMEOUT_SECS;
        rightPanel.timeoutSecs = RIGHT_PANEL_TIMEOUT_SECS;
    }
    
    tonemapShader.load();
    
    // Composite quad mesh (sized to composite dimensions)
    compositeQuadMesh.setMode(OF_PRIMITIVE_TRIANGLE_FAN);
    compositeQuadMesh.getVertices() = {
        { 0.0f, 0.0f, 0.0f },
        { size.x, 0.0f, 0.0f },
        { size.x, size.y, 0.0f },
        { 0.0f, size.y, 0.0f },
    };
    compositeQuadMesh.getTexCoords() = {
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 1.0f, 1.0f },
        { 0.0f, 1.0f },
    };
    
    // Unit quad mesh (for side panels)
    unitQuadMesh.setMode(OF_PRIMITIVE_TRIANGLE_FAN);
    unitQuadMesh.getVertices() = {
        { 0.0f, 0.0f, 0.0f },
        { 1.0f, 0.0f, 0.0f },
        { 1.0f, 1.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f },
    };
    unitQuadMesh.getTexCoords() = {
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 1.0f, 1.0f },
        { 0.0f, 1.0f },
    };
}

void CompositeRenderer::updateCompositeBase(const CompositeParams& params) {
    // Collect layers and separate base from overlay
    std::vector<LayerInfo> baseLayers;
    overlayLayers.clear();
    
    const auto& drawingLayers = params.layers.getLayers();
    auto& alphaParams = params.layers.getAlphaParameterGroup();
    
    size_t i = 0;
    for (const auto& [name, dlptr] : drawingLayers) {
        if (!dlptr->isDrawn) continue;
        float layerAlpha = alphaParams.getFloat(i);
        ++i;
        if (layerAlpha == 0.0f) continue;
        
        float finalAlpha = layerAlpha * params.hibernationAlpha;
        if (finalAlpha == 0.0f) continue;
        
        if (dlptr->isOverlay) {
            overlayLayers.push_back({ dlptr, finalAlpha });
        } else {
            baseLayers.push_back({ dlptr, finalAlpha });
        }
    }
    
    // Phase 1: Clear background and draw base layers
    compositeFbo.begin();
    {
        ofFloatColor bgColor = params.backgroundColor;
        bgColor *= params.backgroundMultiplier;
        bgColor.a = 1.0f;
        ofClear(bgColor);
        
        for (const auto& info : baseLayers) {
            ofEnableBlendMode(info.layer->blendMode);
            ofSetColor(ofFloatColor { 1.0f, 1.0f, 1.0f, info.finalAlpha });
            info.layer->fboPtr->draw(0, 0, compositeFbo.getWidth(), compositeFbo.getHeight());
        }
    }
    compositeFbo.end();
}

void CompositeRenderer::updateCompositeOverlays(const CompositeParams& params) {
    // Phase 2: Draw overlay layers on top (called after mods have rendered their overlays)
    if (overlayLayers.empty()) return;
    
    compositeFbo.begin();
    {
        for (const auto& info : overlayLayers) {
            ofEnableBlendMode(info.layer->blendMode);
            ofSetColor(ofFloatColor { 1.0f, 1.0f, 1.0f, info.finalAlpha });
            info.layer->fboPtr->draw(0, 0, compositeFbo.getWidth(), compositeFbo.getHeight());
        }
    }
    compositeFbo.end();
}

void CompositeRenderer::updateSidePanels() {
    if (panelWidth <= 0.0f) return;
    
    float currentTime = ofGetElapsedTimef();
    
    // Left panel
    if (currentTime - leftPanel.lastUpdateTime > leftPanel.timeoutSecs) {
        leftPanel.lastUpdateTime = currentTime;
        leftPanel.fbo.swap();
        glm::vec2 origin = randomCentralRectOrigin({ panelWidth, panelHeight }, 
                                                    { compositeFbo.getWidth(), compositeFbo.getHeight() });
        leftPanel.fbo.getSource().begin();
        compositeFbo.getTexture().drawSubsection(0.0f, 0.0f, panelWidth, panelHeight, origin.x, origin.y);
        leftPanel.fbo.getSource().end();
    }
    
    // Right panel
    if (currentTime - rightPanel.lastUpdateTime > rightPanel.timeoutSecs) {
        rightPanel.lastUpdateTime = currentTime;
        rightPanel.fbo.swap();
        glm::vec2 origin = randomCentralRectOrigin({ panelWidth, panelHeight },
                                                    { compositeFbo.getWidth(), compositeFbo.getHeight() });
        rightPanel.fbo.getSource().begin();
        compositeFbo.getTexture().drawSubsection(0.0f, 0.0f, panelWidth, panelHeight, origin.x, origin.y);
        rightPanel.fbo.getSource().end();
    }
}

void CompositeRenderer::draw(float windowWidth, float windowHeight,
                              const DisplayController::Settings& mainDisplay,
                              const DisplayController::Settings& sidePanelDisplay,
                              const ConfigTransitionManager* transition) {
    ofEnableBlendMode(OF_BLENDMODE_DISABLED);
    drawSidePanels(0.0f, windowWidth - panelWidth, panelWidth, panelHeight, sidePanelDisplay);
    drawMiddlePanel(windowWidth, windowHeight, scale, mainDisplay, transition);
}

void CompositeRenderer::drawToFbo(ofFbo& target,
                                   const DisplayController::Settings& mainDisplay,
                                   const DisplayController::Settings& sidePanelDisplay,
                                   const ConfigTransitionManager* transition) {
    float fboScale = target.getHeight() / compositeFbo.getHeight();
    float fboSidePanelWidth = (target.getWidth() - compositeFbo.getWidth() * fboScale) / 2.0f;
    
    drawSidePanels(0.0f, target.getWidth() - fboSidePanelWidth, fboSidePanelWidth, panelHeight, sidePanelDisplay);
    drawMiddlePanel(target.getWidth(), target.getHeight(), fboScale, mainDisplay, transition);
}

void CompositeRenderer::drawMiddlePanel(float w, float h, float drawScale,
                                         const DisplayController::Settings& display,
                                         const ConfigTransitionManager* transition) {
    ofPushMatrix();
    ofTranslate((w - compositeFbo.getWidth() * drawScale) / 2.0f, 
                (h - compositeFbo.getHeight() * drawScale) / 2.0f);
    ofScale(drawScale, drawScale);
    
    if (transition && transition->isTransitioning() && transition->hasValidSnapshot()) {
        const auto& snapshotTexData = transition->getSnapshotFbo().getTexture().getTextureData();
        const auto& liveTexData = compositeFbo.getTexture().getTextureData();
        
        beginTonemapShader(display,
                           transition->getSnapshotWeight(),
                           transition->getLiveWeight(),
                           snapshotTexData.bFlipTexture,
                           liveTexData.bFlipTexture,
                           transition->getSnapshotFbo().getTexture(),
                           compositeFbo.getTexture());
    } else {
        const auto& texData = compositeFbo.getTexture().getTextureData();
        
        beginTonemapShader(display,
                           0.0f,
                           1.0f,
                           texData.bFlipTexture,
                           texData.bFlipTexture,
                           compositeFbo.getTexture(),
                           compositeFbo.getTexture());
    }
    
    ofSetColor(255);
    compositeQuadMesh.draw();
    tonemapShader.end();
    
    ofPopMatrix();
}

void CompositeRenderer::drawSidePanels(float xleft, float xright, float w, float h,
                                        const DisplayController::Settings& display) {
    if (panelWidth <= 0.0f) return;
    
    drawPanel(leftPanel, xleft, w, h, display);
    drawPanel(rightPanel, xright, w, h, display);
}

void CompositeRenderer::drawPanel(SidePanel& panel, float x, float w, float h,
                                   const DisplayController::Settings& display) {
    if (panel.timeoutSecs <= 0.0f) return;
    
    float cycleElapsed = (ofGetElapsedTimef() - panel.lastUpdateTime) / panel.timeoutSecs;
    float alphaIn = easeInCubic(ofClamp(cycleElapsed, 0.0f, 1.0f));
    
    const auto& oldTexData = panel.fbo.getTarget().getTexture().getTextureData();
    const auto& newTexData = panel.fbo.getSource().getTexture().getTextureData();
    
    beginTonemapShader(display,
                       1.0f - alphaIn,
                       alphaIn,
                       oldTexData.bFlipTexture,
                       newTexData.bFlipTexture,
                       panel.fbo.getTarget().getTexture(),
                       panel.fbo.getSource().getTexture());
    
    ofPushMatrix();
    ofTranslate(x, 0.0f);
    ofScale(w, h);
    ofSetColor(255);
    unitQuadMesh.draw();
    ofPopMatrix();
    
    tonemapShader.end();
}

void CompositeRenderer::beginTonemapShader(const DisplayController::Settings& display,
                                           float weightA,
                                           float weightB,
                                           bool flipTextureA,
                                           bool flipTextureB,
                                           const ofTexture& textureA,
                                           const ofTexture& textureB) {
    tonemapShader.begin(display.toneMapType,
                        display.exposure,
                        display.gamma,
                        display.whitePoint,
                        display.contrast,
                        display.saturation,
                        display.brightness,
                        display.hueShift,
                        weightA,
                        weightB,
                        flipTextureA,
                        flipTextureB,
                        textureA,
                        textureB);
}

} // namespace ofxMarkSynth
