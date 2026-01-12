# AGENTS.md — ofxMarkSynth

## Build & Run
- Build (in any example dir): `make` | Clean: `make clean && make` | Debug: `make Debug` | Release: `make Release`
- Xcode: open the example `.xcodeproj` and build the target (Debug/Release)
- Single-run: `make -C example_simple run` (if supported); otherwise launch app binary in `bin/`
- ImGui GUI: not all examples enable ImGui. `example_simple` does not use the full ImGui GUI; `example_memory_collage` does.
- For validating ImGui changes in real usage, build/run a host app that embeds the addon (e.g. `apps/myApps/fingerprint2`).
- Tests: none provided; create C++ tests under `tests/` if needed

## Code Style
- Lint/format: clang-format (LLVM style, 120 col). Apply: `clang-format -i $(git ls-files "*.{h,hpp,hh,c,cc,cpp}")`
- Static checks: clang-tidy allowed; prefer modernize/readability checks
- Headers: use `#pragma once`; group includes as: system, openFrameworks, project; keep minimal
- Namespace: all code under `namespace ofxMarkSynth { ... }` (avoid using-directives in headers)

## Naming Conventions
- Classes: CamelCase (`AudioDataSourceMod`)
- Functions/vars: camelCase (`minPitchParameter`)
- Constants: ALL_CAPS with underscores, prefer `constexpr` (`SOURCE_FBO_BEGIN`)

## Source Directory Structure
```
src/
├── config/        # ModFactory, SynthConfigSerializer, PerformanceNavigator
├── controller/    # HibernationController, IntentController, TimeTracker, etc.
├── core/          # Synth, Mod, Gui, Intent, MemoryBank, ParamController
├── gui/           # FontCache, ImGuiUtil, HelpContent, GuiConstants.h
├── layerMods/     # FadeMod, FluidMod, SmearMod
├── nodeEditor/    # NodeEditorLayout, NodeEditorModel, NodeRenderUtil
├── rendering/     # CompositeRenderer, VideoRecorder, AsyncImageSaver
├── sinkMods/      # Output modules (ParticleSetMod, TextMod, etc.)
├── sourceMods/    # Input modules (AudioDataSourceMod, VideoFlowSourceMod, etc.)
└── util/          # Lerp.h, Oklab.h, TimeStringUtil.h
```

## File Organization Conventions
- Constants: dedicated `*Constants.h` files (SynthConstants.h, RenderingConstants.h, GuiConstants.h)
- One class per file; small inlines in headers, impl in `.cpp`
- Keep headers free of heavy deps when possible
- Helper methods are private in class declarations
- Constructor init helpers named `initXxx()`
- JSON helper functions use pattern `getJsonType(json, key, defaultValue)`

## JSON Config Conventions
- **Underscore-prefixed keys (`_key`)**: Reserved for comments in JSON config files. These are skipped when applying config to parameters. Example: `"_comment": "This explains the config"`
- **Constructor config keys**: Config values needed at Mod construction time (not runtime parameters) should be read and erased in `ModFactory.cpp` before passing config to the Mod constructor. This prevents "unknown parameter" errors. Example: `TriggerBased` in PathMod is read in ModFactory, erased from config, and passed as a constructor argument.
- **ResourceManager**: For external dependencies (file paths, device IDs) passed from the application. These are not stored in JSON config.
- **Regular config keys**: Mapped to `ofParameter` values via `Mod::getParameterGroup()`. These appear in the GUI and can be controlled by the Intent system.

## Types & Error Handling
- Prefer modern types: `std::shared_ptr`, `std::unique_ptr`, `glm::vec*`, `ofFloatColor`
- Pass by const ref; use `auto` judiciously
- Prefer strongly-typed enums and `enum class`; avoid magic numbers; document units/ranges
- Errors: use `ofLogError("ofxMarkSynth") << context;` return early on failure
- Check null/shared_ptr before use; validate external resources (textures/FBO/audio)

## Formatting
- Opening braces on same line
- Space after control keywords (`if (x)`)
- 4-space indent
- Trailing commas allowed where supported

## Performance & Threading
- Guard shared state; avoid heavy GL calls off main thread
- Pre-allocate containers; avoid per-frame allocations
- Batch rendering via `ofFbo` / `ofVbo` / `ofVboMesh`

## Dependencies
`ofxIntrospector`, `ofxGui`, `ofxAudioData`, `ofxRenderer`, `ofxDividedArea`, `ofxMotionFromVideo`, `ofxParticleSet`, `ofxPlottable`, `ofxPointClusters`, `ofxSomPalette`, `ofxConvexHull`, `ofxTimeMeasurements`

## Git
- No secrets; small, purposeful commits; build examples before PRs
- Copilot/Cursor rules: none found; add to `.github/copilot-instructions.md` or `.cursor/rules/` if introduced
