# CURRENT-STATE.md

## PROJECT STATUS

MarkSynth is a proof of concept that remains functional for live performances. The codebase has undergone significant refactoring to improve maintainability and organization.

---

## COMPLETED REFACTORING (December 2024)

### Phase 1: Directory Reorganization

Reorganized `src/` from a flat structure into categorical subdirectories:

| Directory | Purpose | Files |
|-----------|---------|-------|
| `config/` | Configuration, serialization, factory | 5 |
| `controller/` | Runtime state controllers | 7 |
| `core/` | Synth, Mod, Gui, Intent, MemoryBank | 7+ |
| `gui/` | ImGui utilities, fonts, help content | 4 |
| `layerMods/` | Layer effect modules (Fade, Fluid, Smear) | 3 |
| `nodeEditor/` | Node graph editor components | 4 |
| `rendering/` | Compositing, video recording, async export | 6 |
| `sinkMods/` | Output rendering modules | 9 |
| `sourceMods/` | Input/data source modules | 8 |
| `util/` | Small utilities (Lerp, Oklab, TimeString) | 3 |

### Phase 2: Code Quality Improvements

#### Constants Extraction
Created dedicated constants headers to eliminate magic numbers:
- `src/core/SynthConstants.h` - MOD_ID_START, FLOAT_EPSILON, MAX_INTENT_SLOTS
- `src/rendering/RenderingConstants.h` - DEFAULT_VIDEO_FPS, PANEL_TIMEOUT_SECS, PBO constants
- `src/gui/GuiConstants.h` - Thumbnail sizes, slider dimensions, button sizes

#### Helper Method Extraction
Refactored long methods into focused helpers:

**ModFactory.cpp:**
- `registerSourceMods()`, `registerProcessMods()`, `registerLayerMods()`, `registerSinkMods()`

**Gui.cpp:**
- `drawNavigationButton()` - PREV/NEXT button rendering
- `drawDisabledSlider()` - Disabled slider styling for intent slots

**CompositeRenderer.cpp:**
- `beginTonemapShader()` - Consolidated duplicate shader setup

**NodeEditorLayout.cpp:**
- `applyRepulsionForces()`, `applySpringForces()`, `applyCenterAttraction()`

**NodeRenderUtil.cpp:**
- `finishParameterRow()` - Common row-finishing pattern

**SynthConfigSerializer.cpp:**
- `getJsonBool()`, `getJsonFloat()`, `getJsonInt()`, `getJsonString()` - JSON helpers

**Synth.cpp:**
- `initControllers()`, `initRendering()`, `initResourcePaths()`, `initPerformanceNavigator()`, `initSinkSourceMappings()` - Constructor helpers
- `updateModConfigJson()` - Config update helper

---

## ARCHITECTURAL NOTES

### Established Conventions
- Constants go in `*Constants.h` files when there are many; otherwise inline in relevant header
- Use `constexpr` for compile-time constants
- Helper methods are private in class declarations
- Use singular folder names (controller/, gui/, rendering/)
- JSON helper functions use pattern `getJsonType(json, key, defaultValue)`
- Constructor init helpers named `initXxx()`

### Build Commands
```bash
# Build any example
make -C example_simple

# Clean build
make -C example_simple clean && make -C example_simple

# Debug build
make -C example_simple Debug
```

---

## NEXT PRIORITIES

### Potential Further Refactoring
Long methods identified for possible extraction:

| File | Method | Lines | Opportunity |
|------|--------|-------|-------------|
| Gui.cpp | `drawNodeEditor()` | 176 | Link drawing, layout handling |
| Gui.cpp | `drawPerformanceNavigator()` | 138 | List items, progress bar |
| AudioDataSourceMod.cpp | `drawEventDetectionOverlay()` | 142 | Detector rows, legend |
| Gui.cpp | `drawSnapshotControls()` | 111 | Per-slot button logic |
| Synth.cpp | `update()` | 104 | updateMods(), updateComposites() |

### Feature Development
- Node editor improvements for connecting and creating synth configurations
- Tune Mod parameters, Intent settings, and auto-response to prevent chaotic visuals
- Expand expressive potential of each Mod
- GPU-based implementation of ofxParticleSet addon
- Build additional MarkSynth configurations from different Mods and connections
