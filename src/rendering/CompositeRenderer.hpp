//
//  CompositeRenderer.hpp
//  ofxMarkSynth
//
//  Manages layer compositing, side panels, and final output rendering.
//

#pragma once

#include "controller/DisplayController.hpp"
#include "controller/LayerController.hpp"
#include "controller/ConfigTransitionManager.hpp"
#include "rendering/TonemapCrossfadeShader.h"
#include "PingPongFbo.h"
#include "ofFbo.h"
#include "ofMesh.h"

namespace ofxMarkSynth {

/// Renders the composite image from layers and handles display output.
class CompositeRenderer {
public:
    CompositeRenderer() = default;

    /// Allocate FBOs. Set panelGapPx to disable side panels if <= 0 after calculation.
    void allocate(glm::vec2 compositeSize, float windowWidth, float windowHeight, float panelGapPx);

    /// Parameters for composite update
    struct CompositeParams {
        LayerController& layers;
        float hibernationAlpha;         // 1.0 = fully visible, 0.0 = hibernated
        ofFloatColor backgroundColor;
        float backgroundMultiplier;
    };

    /// Update composite: Phase 1 - clear background and draw base layers
    void updateCompositeBase(const CompositeParams& params);

    /// Update composite: Phase 2 - draw overlay layers on top
    /// Call this after mods have rendered their overlays
    void updateCompositeOverlays(const CompositeParams& params);

    /// Update side panels with new crops from composite (call each frame)
    void updateSidePanels();

    /// Draw to screen
    void draw(float windowWidth, float windowHeight,
              const DisplayController::Settings& mainDisplay,
              const DisplayController::Settings& sidePanelDisplay,
              const ConfigTransitionManager* transition);

    /// Draw to FBO (for video recording)
    void drawToFbo(ofFbo& target,
                   const DisplayController::Settings& mainDisplay,
                   const DisplayController::Settings& sidePanelDisplay,
                   const ConfigTransitionManager* transition);

    // Accessors
    const ofFbo& getCompositeFbo() const { return compositeFbo; }
    glm::vec2 getCompositeSize() const { return size; }
    float getCompositeScale() const { return scale; }
    bool hasSidePanels() const { return panelWidth > 0.0f; }
    float getSidePanelWidth() const { return panelWidth; }
    float getSidePanelHeight() const { return panelHeight; }

private:
    // Composite FBO
    ofFbo compositeFbo;
    glm::vec2 size { 0, 0 };
    float scale { 1.0f };

    // Side panels
    struct SidePanel {
        PingPongFbo fbo;
        float lastUpdateTime { 0.0f };
        float timeoutSecs { 7.0f };
    };
    SidePanel leftPanel;
    SidePanel rightPanel;
    float panelWidth { 0.0f };
    float panelHeight { 0.0f };

    // Shader and meshes
    TonemapCrossfadeShader tonemapShader;
    ofMesh compositeQuadMesh;
    ofMesh unitQuadMesh;

    // Cached overlay layers for phase 2
    struct LayerInfo {
        DrawingLayerPtr layer;
        float finalAlpha;
    };
    std::vector<LayerInfo> overlayLayers;

    // Internal draw helpers
    void drawMiddlePanel(float w, float h, float drawScale,
                         const DisplayController::Settings& display,
                         const ConfigTransitionManager* transition);
    void drawSidePanels(float xleft, float xright, float w, float h,
                        const DisplayController::Settings& display);
    void drawPanel(SidePanel& panel, float x, float w, float h,
                   const DisplayController::Settings& display);
    
    /// Begin tonemap shader with display settings and crossfade textures
    void beginTonemapShader(const DisplayController::Settings& display,
                             float weightA,
                             float weightB,
                             bool flipTextureA,
                             bool flipTextureB,
                             const ofTexture& textureA,
                             const ofTexture& textureB);
};

} // namespace ofxMarkSynth
