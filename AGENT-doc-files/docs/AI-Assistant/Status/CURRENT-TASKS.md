# CURRENT-TASKS.md

## **COMPLETED**

### December 2024 - Codebase Refactoring

- [x] **Directory Reorganization**: Split flat `src/` into categorical subdirectories (config/, controller/, core/, gui/, layerMods/, nodeEditor/, rendering/, sinkMods/, sourceMods/, util/)

- [x] **Constants Extraction**: Created `SynthConstants.h`, `RenderingConstants.h`, `GuiConstants.h` to eliminate magic numbers

- [x] **ModFactory Refactoring**: Split `initializeBuiltinTypes()` into `registerSourceMods()`, `registerProcessMods()`, `registerLayerMods()`, `registerSinkMods()`

- [x] **Gui.cpp Refactoring**: Extracted `drawNavigationButton()` and `drawDisabledSlider()` helpers

- [x] **CompositeRenderer Refactoring**: Extracted `beginTonemapShader()` to consolidate duplicate shader setup

- [x] **NodeEditorLayout Refactoring**: Split `applyForces()` into `applyRepulsionForces()`, `applySpringForces()`, `applyCenterAttraction()`

- [x] **NodeRenderUtil Refactoring**: Extracted `finishParameterRow()` helper

- [x] **SynthConfigSerializer Refactoring**: Added JSON helpers (`getJsonBool`, `getJsonFloat`, `getJsonInt`, `getJsonString`)

- [x] **Synth Constructor Refactoring**: Split into `initControllers()`, `initRendering()`, `initResourcePaths()`, `initPerformanceNavigator()`, `initSinkSourceMappings()`

- [x] **Synth::saveModsToCurrentConfig Refactoring**: Extracted `updateModConfigJson()` helper

- [x] **Documentation Update**: Updated all AGENT-doc-files to reflect new structure

## **NEXT PRIORITIES**

### Optional Further Refactoring

- [ ] `Gui.cpp::drawNodeEditor()` (176 lines) - Extract link drawing, layout handling
- [ ] `Gui.cpp::drawPerformanceNavigator()` (138 lines) - Extract list items, progress bar
- [ ] `AudioDataSourceMod.cpp::drawEventDetectionOverlay()` (142 lines) - Extract detector rows, legend
- [ ] `Gui.cpp::drawSnapshotControls()` (111 lines) - Extract per-slot button logic
- [ ] `Synth.cpp::update()` (104 lines) - Extract updateMods(), updateComposites()

### Feature Development

- [ ] Node editor improvements for synth configuration
- [ ] Tune Mod parameters and Intent settings for visual stability
- [ ] GPU-based implementation of ofxParticleSet
