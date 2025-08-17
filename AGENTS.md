# AGENTS.md - ofxMarkSynth

## Build Commands
- **Build project**: `make` (in any example directory)
- **Clean build**: `make clean && make`
- **Debug build**: `make Debug`
- **Release build**: `make Release`

## Code Style Guidelines
- **Headers**: Use `#pragma once` for header guards
- **Namespacing**: All code in `ofxMarkSynth` namespace
- **Naming**: CamelCase for classes (`AudioDataSourceMod`), camelCase for variables/functions (`minPitchParameter`)
- **Types**: Use modern C++ types (`std::shared_ptr`, `glm::vec2/3/4`, `ofFloatColor`)
- **Constants**: Use `constexpr` for compile-time constants, ALL_CAPS naming (`SOURCE_FBO_BEGIN`)
- **Includes**: System includes first, then openFrameworks, then project headers
- **Braces**: Opening brace on same line for functions/classes
- **Spacing**: Space after control flow keywords (`if (condition)`)

## Error Handling
- Use `ofLogError()` for error logging with context
- Check for null pointers before use (`if (!audioDataProcessorPtr)`)
- Return early from functions on error conditions

## Dependencies
Core addon dependencies: `ofxIntrospector`, `ofxGui`, `ofxAudioData`, `ofxRenderer`, `ofxDividedArea`, `ofxMotionFromVideo`, `ofxParticleSet`, `ofxPlottable`, `ofxPointClusters`, `ofxSomPalette`, `ofxConvexHull`, `ofxTimeMeasurements`