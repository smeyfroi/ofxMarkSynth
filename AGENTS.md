# AGENTS.md — ofxMarkSynth

- Build (in any example dir): `make` • Clean: `make clean && make` • Debug: `make Debug` • Release: `make Release`
- Xcode: open the example `.xcodeproj` and build the target (Debug/Release)
- Tests: none provided; create C++ tests under `tests/` if needed. To run a single example headless, use `make -C example_simple` (replace dir as needed)
- Lint/format: use clang-format (LLVM style, 120 col). Apply: `clang-format -i $(git ls-files "*.{h,hpp,hh,c,cc,cpp}")`
- Static checks: `clang-tidy` allowed; prefer modernize/readability checks when adding CI
- Headers: use `#pragma once`; group includes as: system, openFrameworks, project; keep includes minimal
- Namespace: all code under `namespace ofxMarkSynth { ... }` (avoid using-directives in headers)
- Naming: Classes CamelCase (`AudioDataSourceMod`); functions/vars camelCase (`minPitchParameter`); constants ALL_CAPS with underscores, prefer `constexpr` (`SOURCE_FBO_BEGIN`)
- Types: prefer modern types (`std::shared_ptr`, `std::unique_ptr`, `glm::vec*`, `ofFloatColor`); pass by const ref; use `auto` judiciously
- Formatting: opening braces on same line; space after control keywords (`if (x)`); 4-space indent; trailing commas allowed where supported
- Errors: use `ofLogError("ofxMarkSynth") << context;` return early on failure; check null/shared_ptr before use; validate external resources (textures/FBO/audio)
- Threading: guard shared state; avoid heavy GL calls off main thread
- Parameters: prefer strongly-typed enums and `enum class`; avoid magic numbers; document units/ranges
- File layout: one class per file; small inlines in headers, impl in `.cpp`; keep headers free of heavy deps when possible
- Dependencies: `ofxIntrospector`, `ofxGui`, `ofxAudioData`, `ofxRenderer`, `ofxDividedArea`, `ofxMotionFromVideo`, `ofxParticleSet`, `ofxPlottable`, `ofxPointClusters`, `ofxSomPalette`, `ofxConvexHull`, `ofxTimeMeasurements`
- Git: no secrets; small, purposeful commits; build examples before PRs
- Single-run tip: `make -C example_particles_from_video run` (if example supports `run` target); otherwise launch app binary in `bin/`
- Copilot/Cursor rules: none found in `.github/copilot-instructions.md` or `.cursor/rules/`; add if introduced later
