# TODO.md

## DO IT NOW:

- [ ] (No immediate tasks - refactoring completed)

## REMEMBER:

- Some `..\..\addons` files are part of the MarkSynth codebase so ask me before modifying them
- Follow established conventions (see AGENTS.md):
  - Constants in `*Constants.h` files
  - Helper methods as private class members
  - Constructor init helpers named `initXxx()`
  - JSON helpers use pattern `getJsonType(json, key, defaultValue)`

## OPTIONAL REFACTORING:

Long methods that could be further extracted:
- `Gui.cpp::drawNodeEditor()` (176 lines)
- `Gui.cpp::drawPerformanceNavigator()` (138 lines)
- `AudioDataSourceMod.cpp::drawEventDetectionOverlay()` (142 lines)
- `Gui.cpp::drawSnapshotControls()` (111 lines)
- `Synth.cpp::update()` (104 lines)

# DONE. CHANGELOG:

## December 2024 - Codebase Refactoring

- [x] Directory reorganization: src/ split into 10 categorical subdirectories
- [x] Constants extraction: Created SynthConstants.h, RenderingConstants.h, GuiConstants.h
- [x] ModFactory refactoring: Split registration into grouped methods
- [x] Gui.cpp refactoring: Extracted drawNavigationButton(), drawDisabledSlider()
- [x] CompositeRenderer refactoring: Extracted beginTonemapShader()
- [x] NodeEditorLayout refactoring: Split applyForces() into force-specific methods
- [x] NodeRenderUtil refactoring: Extracted finishParameterRow()
- [x] SynthConfigSerializer refactoring: Added JSON helper functions
- [x] Synth constructor refactoring: Split into initXxx() methods
- [x] Synth::saveModsToCurrentConfig refactoring: Extracted updateModConfigJson()
- [x] Documentation updated to reflect new structure
