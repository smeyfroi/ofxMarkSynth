//
//  MemoryBankController.hpp
//  ofxMarkSynth
//
//  Controller wrapping MemoryBank with parameters, sink handling, and intent application.
//  Extracted from Synth to reduce its responsibilities.
//

#pragma once

#include "core/MemoryBank.hpp"
#include "core/ParamController.h"
#include "ofParameter.h"
#include <filesystem>
#include <map>
#include <string>

namespace ofxMarkSynth {

class Intent;

class MemoryBankController {
public:
    // Sink ID constants (moved from Synth)
    static constexpr int SINK_MEMORY_SAVE = 300;
    static constexpr int SINK_MEMORY_SAVE_SLOT = 301;
    static constexpr int SINK_MEMORY_EMIT = 302;
    static constexpr int SINK_MEMORY_EMIT_SLOT = 303;
    static constexpr int SINK_MEMORY_EMIT_RANDOM = 304;
    static constexpr int SINK_MEMORY_EMIT_RANDOM_NEW = 305;
    static constexpr int SINK_MEMORY_EMIT_RANDOM_OLD = 306;
    static constexpr int SINK_MEMORY_SAVE_CENTRE = 307;
    static constexpr int SINK_MEMORY_SAVE_WIDTH = 308;
    static constexpr int SINK_MEMORY_EMIT_CENTRE = 309;
    static constexpr int SINK_MEMORY_EMIT_WIDTH = 310;
    static constexpr int SINK_MEMORY_CLEAR_ALL = 311;

    /// Result of handling an emit-type sink
    struct EmitResult {
        const ofTexture* texture { nullptr };
        bool shouldEmit { false };
    };

    MemoryBankController();

    /// Allocate the memory bank FBOs
    void allocate(glm::vec2 memorySize);

    /// Build the parameter group (call after allocation)
    void buildParameterGroup();

    /// Handle a sink message. Returns emit result for emit-type sinks.
    /// @param sinkId The sink identifier
    /// @param value The received value
    /// @param compositeFbo The current composite FBO (for save operations)
    /// @param agency Current agency value for parameter controllers
    /// @return EmitResult with texture pointer if an emit should occur
    EmitResult handleSink(int sinkId, float value, const ofFbo& compositeFbo, float agency);

    /// Apply intent to memory parameters
    void applyIntent(const Intent& intent, float intentStrength);

    /// Update: process pending saves and save-all requests
    /// @param compositeFbo The current composite FBO
    /// @param configRootPath Root path for saving global memories (empty to skip save-all)
    void update(const ofFbo& compositeFbo, const std::filesystem::path& configRootPath);

    /// Load global memories from disk (call once after first config load)
    /// @return true if loading was attempted (regardless of success)
    bool loadGlobalMemories(const std::filesystem::path& configRootPath);

    /// Request saving all memories to disk (processed in next update)
    void requestSaveAll();

    /// Get sink name -> ID mapping for Mod system registration
    std::map<std::string, int> getSinkNameIdMap() const;

    /// Direct access to the underlying MemoryBank (for Gui)
    MemoryBank& getMemoryBank() { return memoryBank; }
    const MemoryBank& getMemoryBank() const { return memoryBank; }

    /// Parameter group accessor
    ofParameterGroup& getParameterGroup() { return parameters; }

private:
    MemoryBank memoryBank;
    bool globalMemoryBankLoaded { false };
    bool memorySaveAllRequested { false };

    // Parameters
    ofParameterGroup parameters;

    // Memory save parameters
    ofParameter<float> saveCentreParameter { "MemorySaveCentre", 1.0, 0.0, 1.0 };
    ofParameter<float> saveWidthParameter { "MemorySaveWidth", 0.0, 0.0, 1.0 };
    ParamController<float> saveCentreController { saveCentreParameter };
    ParamController<float> saveWidthController { saveWidthParameter };

    // Memory emit parameters
    ofParameter<float> emitCentreParameter { "MemoryEmitCentre", 0.5, 0.0, 1.0 };
    ofParameter<float> emitWidthParameter { "MemoryEmitWidth", 1.0, 0.0, 1.0 };
    ParamController<float> emitCentreController { emitCentreParameter };
    ParamController<float> emitWidthController { emitWidthParameter };

    // Emit rate limiting
    float lastEmitTime { 0.0f };
    ofParameter<float> emitMinIntervalParameter { "MemoryEmitMinInterval", 0.1, 0.0, 2.0 };

    /// Check rate limit and return texture if emit is allowed
    EmitResult emitWithRateLimit(const ofTexture* tex);
};

} // namespace ofxMarkSynth
