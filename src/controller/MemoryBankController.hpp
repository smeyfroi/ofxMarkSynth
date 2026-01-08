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
#include <array>
#include <filesystem>
#include <map>
#include <memory>
#include <string>

namespace ofxMarkSynth {

class Intent;
struct MemoryBankAutoCaptureState;

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

    // Legacy manual save-selection controls. These remain for backwards compatibility,
    // but are not used by the built-in auto-capture policy.
    static constexpr int SINK_MEMORY_SAVE_CENTRE = 307;
    static constexpr int SINK_MEMORY_SAVE_WIDTH = 308;

    static constexpr int SINK_MEMORY_EMIT_CENTRE = 309;
    static constexpr int SINK_MEMORY_EMIT_WIDTH = 310;
    static constexpr int SINK_MEMORY_CLEAR_ALL = 311;

    // Auto-capture controls
    static constexpr int SINK_MEMORY_AUTO_CAPTURE_ENABLED = 312;
    static constexpr int SINK_MEMORY_AUTO_CAPTURE_WARMUP_TARGET_SEC = 313;
    static constexpr int SINK_MEMORY_AUTO_CAPTURE_WARMUP_INTERVAL_SEC = 314;
    static constexpr int SINK_MEMORY_AUTO_CAPTURE_RETRY_INTERVAL_SEC = 315;
    static constexpr int SINK_MEMORY_AUTO_CAPTURE_RECENT_INTERVAL_SEC = 316;
    static constexpr int SINK_MEMORY_AUTO_CAPTURE_MID_INTERVAL_SEC = 317;
    static constexpr int SINK_MEMORY_AUTO_CAPTURE_LONG_INTERVAL_SEC = 318;
    static constexpr int SINK_MEMORY_AUTO_CAPTURE_MIN_VARIANCE = 319;
    static constexpr int SINK_MEMORY_AUTO_CAPTURE_MIN_ACTIVE_FRACTION = 320;

    static constexpr int SINK_MEMORY_AUTO_CAPTURE_QUALITY_FLOOR = 321;
    static constexpr int SINK_MEMORY_AUTO_CAPTURE_ANCHOR_QUALITY_FLOOR = 322;
    static constexpr int SINK_MEMORY_AUTO_CAPTURE_LOW_QUALITY_RETRY_SEC = 323;
    static constexpr int SINK_MEMORY_AUTO_CAPTURE_ABS_IMPROVE = 324;
    static constexpr int SINK_MEMORY_AUTO_CAPTURE_REL_IMPROVE_RECENT = 325;
    static constexpr int SINK_MEMORY_AUTO_CAPTURE_REL_IMPROVE_MID = 326;
    static constexpr int SINK_MEMORY_AUTO_CAPTURE_REL_IMPROVE_LONG = 327;

    /// Result of handling an emit-type sink
    struct EmitResult {
        const ofTexture* texture { nullptr };
        bool shouldEmit { false };
    };

    struct AutoCaptureSlotDebug {
        bool isOccupied { false };
        int band { 0 }; // 0 = long, 1 = mid, 2 = recent
        bool isAnchorLocked { false };

        float captureTimeSec { -1.0f };
        float nextDueTimeSec { -1.0f };

        float variance { -1.0f };
        float activeFraction { -1.0f };
        float qualityScore { -1.0f };
    };

    MemoryBankController();
    ~MemoryBankController();

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

    /// Update: process pending saves, auto-capture, and save-all requests
    /// @param compositeFbo The current composite FBO
    /// @param configRootPath Root path for saving global memories (empty to skip save-all)
    /// @param synthRunningTimeSec Time since performance start (pauses with synth)
    void update(const ofFbo& compositeFbo,
                const std::filesystem::path& configRootPath,
                float synthRunningTimeSec);

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

    ofParameter<bool>& getAutoCaptureEnabledParameter() { return autoCaptureEnabledParameter; }

    /// Debug-only: returns false if no state exists yet.
    bool getAutoCaptureSlotDebug(int slot, float synthRunningTimeSec, AutoCaptureSlotDebug& out) const;

private:
    MemoryBank memoryBank;
    bool globalMemoryBankLoaded { false };
    bool memorySaveAllRequested { false };

    bool legacySaveSelectionWarningLogged { false };

    float lastSynthRunningTimeSec { 0.0f };

    std::unique_ptr<MemoryBankAutoCaptureState> autoCaptureState;

    // Parameters
    ofParameterGroup parameters;

    // Memory save parameters (LEGACY)
    // These influence `MemorySave` overwrites when the bank is full.
    // The performance-focused auto-capture system ignores these.
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

    // Auto-capture parameters (across performance)
    ofParameter<bool> autoCaptureEnabledParameter { "MemoryAutoCaptureEnabled", true };

    ofParameter<float> autoCaptureWarmupTargetSecParameter { "MemoryAutoCaptureWarmupTargetSec", 120.0, 5.0, 600.0 };
    ofParameter<float> autoCaptureWarmupIntervalSecParameter { "MemoryAutoCaptureWarmupIntervalSec", 6.0, 0.5, 60.0 };
    ofParameter<float> autoCaptureRetryIntervalSecParameter { "MemoryAutoCaptureRetryIntervalSec", 3.0, 0.1, 30.0 };

    ofParameter<float> autoCaptureRecentIntervalSecParameter { "MemoryAutoCaptureRecentIntervalSec", 15.0, 1.0, 300.0 };
    ofParameter<float> autoCaptureMidIntervalSecParameter { "MemoryAutoCaptureMidIntervalSec", 90.0, 5.0, 900.0 };
    ofParameter<float> autoCaptureLongIntervalSecParameter { "MemoryAutoCaptureLongIntervalSec", 600.0, 30.0, 3600.0 };

    // Density heuristics (computed from tiny downsample)
    // Defaults intentionally permissive so auto-capture actually fills the bank in typical scenes.
    ofParameter<float> autoCaptureMinVarianceParameter { "MemoryAutoCaptureMinVariance", 0.0015, 0.0, 0.2 };
    ofParameter<float> autoCaptureMinActiveFractionParameter { "MemoryAutoCaptureMinActiveFraction", 0.02, 0.0, 1.0 };

    // Auto-capture quality policy
    ofParameter<float> autoCaptureQualityFloorParameter { "MemoryAutoCaptureQualityFloor", 0.00015, 0.0, 0.1 };
    ofParameter<float> autoCaptureAnchorQualityFloorParameter { "MemoryAutoCaptureAnchorQualityFloor", 0.00030, 0.0, 0.1 };
    ofParameter<float> autoCaptureLowQualityRetrySecParameter { "MemoryAutoCaptureLowQualityRetrySec", 45.0, 0.5, 600.0 };

    ofParameter<float> autoCaptureAbsImproveParameter { "MemoryAutoCaptureAbsImprove", 0.00005, 0.0, 0.05 };
    ofParameter<float> autoCaptureRelImproveRecentParameter { "MemoryAutoCaptureRelImproveRecent", 0.25, 0.0, 5.0 };
    ofParameter<float> autoCaptureRelImproveMidParameter { "MemoryAutoCaptureRelImproveMid", 0.35, 0.0, 5.0 };
    ofParameter<float> autoCaptureRelImproveLongParameter { "MemoryAutoCaptureRelImproveLong", 0.50, 0.0, 5.0 };

    // Warmup behavior (fill fast)
    ofParameter<int> autoCaptureWarmupBurstCountParameter { "MemoryAutoCaptureWarmupBurstCount", 2, 1, 4 };

    // Analysis resolution (lower == cheaper)
    ofParameter<int> autoCaptureAnalysisSizeParameter { "MemoryAutoCaptureAnalysisSize", 32, 8, 128 };

    /// Check rate limit and return texture if emit is allowed
    EmitResult emitWithRateLimit(const ofTexture* tex);

    void updateAutoCapture(const ofFbo& compositeFbo, float synthRunningTimeSec);

    void noteManualSaveSlot(int slot);
    void logLegacySaveSelectionWarningOnce();

    static int getBandForSlot(int slot);
};

} // namespace ofxMarkSynth
