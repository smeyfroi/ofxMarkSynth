//
//  MemoryBankController.cpp
//  ofxMarkSynth
//
//  Controller wrapping MemoryBank with parameters, sink handling, and intent application.
//

#include "controller/MemoryBankController.hpp"

#include "core/Intent.hpp"
#include "ofGraphics.h"
#include "ofLog.h"
#include "ofMath.h"
#include "ofUtils.h"

#include "ofBufferObject.h"
#include "ofGLUtils.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace ofxMarkSynth {

namespace {

static constexpr int AUTO_CAPTURE_MIN_FRAMES_TO_WAIT = 2;
static constexpr int AUTO_CAPTURE_MAX_FRAMES_BEFORE_ABANDON = 60;

struct DensityMetrics {
    float variance { 0.0f };
    float activeFraction { 0.0f };
};

static float computeLuma01(unsigned char r, unsigned char g, unsigned char b) {
    // Rec. 709 luma
    return (0.2126f * (static_cast<float>(r) / 255.0f))
        + (0.7152f * (static_cast<float>(g) / 255.0f))
        + (0.0722f * (static_cast<float>(b) / 255.0f));
}

static DensityMetrics computeDensityMetricsRgb(const std::vector<unsigned char>& rgb, int size, float activeEpsilon) {
    const int nPix = size * size;
    if (nPix <= 0 || static_cast<int>(rgb.size()) < nPix * 3) {
        return {};
    }

    double mean = 0.0;
    for (int i = 0; i < nPix; ++i) {
        const unsigned char r = rgb[i * 3 + 0];
        const unsigned char g = rgb[i * 3 + 1];
        const unsigned char b = rgb[i * 3 + 2];
        mean += computeLuma01(r, g, b);
    }
    mean /= static_cast<double>(nPix);

    double var = 0.0;
    int activeCount = 0;
    for (int i = 0; i < nPix; ++i) {
        const unsigned char r = rgb[i * 3 + 0];
        const unsigned char g = rgb[i * 3 + 1];
        const unsigned char b = rgb[i * 3 + 2];
        float y = computeLuma01(r, g, b);
        float d = y - static_cast<float>(mean);
        var += static_cast<double>(d) * static_cast<double>(d);

        if (std::abs(d) > activeEpsilon) {
            activeCount++;
        }
    }

    DensityMetrics out;
    out.variance = static_cast<float>(var / static_cast<double>(nPix));
    out.activeFraction = static_cast<float>(activeCount) / static_cast<float>(nPix);
    return out;
}

static float computeQualityScore(const DensityMetrics& metrics) {
    // Simple but effective: rewards both coverage (activeFraction) and texture (variance).
    return metrics.variance * metrics.activeFraction;
}

static float jitteredInterval(float intervalSec, float jitterFraction = 0.15f) {
    float jitter = intervalSec * jitterFraction;
    return std::max(0.0f, intervalSec + ofRandom(-jitter, jitter));
}

static float getTimeBandIntervalSec(int slot,
                                   float recentIntervalSec,
                                   float midIntervalSec,
                                   float longIntervalSec) {
    // Slot layout: 0-2 long, 3-5 mid, 6-7 recent.
    if (slot <= 2) return longIntervalSec;
    if (slot <= 5) return midIntervalSec;
    return recentIntervalSec;
}

} // namespace

struct MemoryBankAutoCaptureState {
    enum class State { IDLE, PBO_WAITING };

    State state { State::IDLE };

    int analysisSize { 0 };
    ofFbo analysisFbo;
    ofBufferObject pbo;

    GLsync fence { nullptr };
    int framesWaited { 0 };

    std::vector<unsigned char> pixels;

    float nextAttemptTimeSec { 0.0f };
    std::vector<float> nextSlotDueTimeSec;

    std::vector<int> pendingSaveSlots;
    glm::vec2 pendingCropTopLeft { 0.0f, 0.0f };
    glm::vec2 pendingCropSize { 0.0f, 0.0f };

    std::array<float, MemoryBank::NUM_SLOTS> slotCaptureTimeSec;
    std::array<float, MemoryBank::NUM_SLOTS> slotVariance;
    std::array<float, MemoryBank::NUM_SLOTS> slotActiveFraction;
    std::array<float, MemoryBank::NUM_SLOTS> slotQualityScore;

    int lockedAnchorSlot { -1 };

    MemoryBankAutoCaptureState() {
        slotCaptureTimeSec.fill(-1.0f);
        slotVariance.fill(-1.0f);
        slotActiveFraction.fill(-1.0f);
        slotQualityScore.fill(-1.0f);
    }
};

int MemoryBankController::getBandForSlot(int slot) {
    if (slot <= 2) return 0;
    if (slot <= 5) return 1;
    return 2;
}

MemoryBankController::MemoryBankController() = default;

MemoryBankController::~MemoryBankController() {
    if (autoCaptureState && autoCaptureState->fence) {
        glDeleteSync(autoCaptureState->fence);
        autoCaptureState->fence = nullptr;
    }
}

void MemoryBankController::allocate(glm::vec2 memorySize) {
    // Memories should be opaque; store as RGB to avoid alpha channel artifacts.
    memoryBank.allocate(memorySize, GL_RGB8);

    autoCaptureState.reset();
}

void MemoryBankController::buildParameterGroup() {
    parameters.clear();
    parameters.setName("MemoryBank");

    parameters.add(saveCentreParameter);
    parameters.add(saveWidthParameter);
    parameters.add(emitCentreParameter);
    parameters.add(emitWidthParameter);
    parameters.add(emitMinIntervalParameter);

    parameters.add(autoCaptureEnabledParameter);
    parameters.add(autoCaptureWarmupTargetSecParameter);
    parameters.add(autoCaptureWarmupIntervalSecParameter);
    parameters.add(autoCaptureRetryIntervalSecParameter);
    parameters.add(autoCaptureRecentIntervalSecParameter);
    parameters.add(autoCaptureMidIntervalSecParameter);
    parameters.add(autoCaptureLongIntervalSecParameter);
    parameters.add(autoCaptureMinVarianceParameter);
    parameters.add(autoCaptureMinActiveFractionParameter);

    parameters.add(autoCaptureQualityFloorParameter);
    parameters.add(autoCaptureAnchorQualityFloorParameter);
    parameters.add(autoCaptureLowQualityRetrySecParameter);
    parameters.add(autoCaptureAbsImproveParameter);
    parameters.add(autoCaptureRelImproveRecentParameter);
    parameters.add(autoCaptureRelImproveMidParameter);
    parameters.add(autoCaptureRelImproveLongParameter);

    parameters.add(autoCaptureWarmupBurstCountParameter);
    parameters.add(autoCaptureAnalysisSizeParameter);
}

MemoryBankController::EmitResult MemoryBankController::emitWithRateLimit(const ofTexture* tex) {
    if (!tex) return { nullptr, false };

    float now = ofGetElapsedTimef();
    if (now - lastEmitTime < emitMinIntervalParameter) {
        return { nullptr, false };
    }

    lastEmitTime = now;
    return { tex, true };
}

void MemoryBankController::logLegacySaveSelectionWarningOnce() {
    if (legacySaveSelectionWarningLogged) return;

    legacySaveSelectionWarningLogged = true;
    ofLogWarning("MemoryBankController")
        << "Legacy: MemorySaveCentre/Width affect manual MemorySave overwrite behavior, but are ignored by auto-capture. "
        << "Consider using MemoryAutoCapture* parameters instead.";
}

void MemoryBankController::noteManualSaveSlot(int slot) {
    // Manual GUI saves (and sink-driven MemorySaveSlot) bypass the auto-capture analysis path.
    // We still record capture time so tooltips/debug make sense, but leave quality unknown so
    // the auto-capture upgrader can replace low-quality placeholders when denser moments arrive.
    if (!autoCaptureState) {
        autoCaptureState = std::make_unique<MemoryBankAutoCaptureState>();
    }

    if (slot < 0 || slot >= MemoryBank::NUM_SLOTS) return;

    MemoryBankAutoCaptureState& state = *autoCaptureState;
    state.slotCaptureTimeSec[slot] = lastSynthRunningTimeSec;
    state.slotVariance[slot] = -1.0f;
    state.slotActiveFraction[slot] = -1.0f;
    state.slotQualityScore[slot] = -1.0f;

    if (state.nextSlotDueTimeSec.size() == MemoryBank::NUM_SLOTS) {
        // Encourage an upgrade pass sooner rather than waiting for the full band cadence.
        state.nextSlotDueTimeSec[slot] = std::min(state.nextSlotDueTimeSec[slot],
                                                 lastSynthRunningTimeSec + autoCaptureLowQualityRetrySecParameter);
    }
}

MemoryBankController::EmitResult MemoryBankController::handleSink(int sinkId,
                                                                 float value,
                                                                 const ofFbo& compositeFbo,
                                                                 float agency) {
    switch (sinkId) {
        case SINK_MEMORY_SAVE:
            if (value > 0.5f) {
                memoryBank.save(compositeFbo, saveCentreController.value, saveWidthController.value);
            }
            return { nullptr, false };

        case SINK_MEMORY_SAVE_SLOT: {
            int slot = static_cast<int>(value) % MemoryBank::NUM_SLOTS;
            memoryBank.saveToSlot(compositeFbo, slot);
            noteManualSaveSlot(slot);
            return { nullptr, false };
        }

        case SINK_MEMORY_EMIT:
            if (value > 0.5f) {
                return emitWithRateLimit(memoryBank.select(emitCentreController.value, emitWidthController.value));
            }
            return { nullptr, false };

        case SINK_MEMORY_EMIT_SLOT: {
            int slot = static_cast<int>(value) % MemoryBank::NUM_SLOTS;
            return emitWithRateLimit(memoryBank.get(slot));
        }

        case SINK_MEMORY_EMIT_RANDOM:
            if (value > 0.0f) {
                return emitWithRateLimit(memoryBank.selectRandom());
            }
            return { nullptr, false };

        case SINK_MEMORY_EMIT_RANDOM_NEW:
            if (value > 0.5f) {
                return emitWithRateLimit(memoryBank.selectWeightedRecent(emitCentreController.value,
                                                                         emitWidthController.value));
            }
            return { nullptr, false };

        case SINK_MEMORY_EMIT_RANDOM_OLD:
            if (value > 0.5f) {
                return emitWithRateLimit(memoryBank.selectWeightedOld(emitCentreController.value, emitWidthController.value));
            }
            return { nullptr, false };

        case SINK_MEMORY_SAVE_CENTRE:
            logLegacySaveSelectionWarningOnce();
            saveCentreController.updateAuto(value, agency);
            return { nullptr, false };

        case SINK_MEMORY_SAVE_WIDTH:
            logLegacySaveSelectionWarningOnce();
            saveWidthController.updateAuto(value, agency);
            return { nullptr, false };

        case SINK_MEMORY_EMIT_CENTRE:
            emitCentreController.updateAuto(value, agency);
            return { nullptr, false };

        case SINK_MEMORY_EMIT_WIDTH:
            emitWidthController.updateAuto(value, agency);
            return { nullptr, false };

        case SINK_MEMORY_CLEAR_ALL:
            if (value > 0.5f) {
                memoryBank.clearAll();
                autoCaptureState.reset();
                legacySaveSelectionWarningLogged = false;
                ofLogNotice("MemoryBankController") << "Memory bank cleared";
            }
            return { nullptr, false };

        case SINK_MEMORY_AUTO_CAPTURE_ENABLED:
            autoCaptureEnabledParameter.set(value > 0.5f);
            return { nullptr, false };

        case SINK_MEMORY_AUTO_CAPTURE_WARMUP_TARGET_SEC:
            autoCaptureWarmupTargetSecParameter.set(std::max(0.0f, value));
            return { nullptr, false };

        case SINK_MEMORY_AUTO_CAPTURE_WARMUP_INTERVAL_SEC:
            autoCaptureWarmupIntervalSecParameter.set(std::max(0.0f, value));
            return { nullptr, false };

        case SINK_MEMORY_AUTO_CAPTURE_RETRY_INTERVAL_SEC:
            autoCaptureRetryIntervalSecParameter.set(std::max(0.0f, value));
            return { nullptr, false };

        case SINK_MEMORY_AUTO_CAPTURE_RECENT_INTERVAL_SEC:
            autoCaptureRecentIntervalSecParameter.set(std::max(0.0f, value));
            return { nullptr, false };

        case SINK_MEMORY_AUTO_CAPTURE_MID_INTERVAL_SEC:
            autoCaptureMidIntervalSecParameter.set(std::max(0.0f, value));
            return { nullptr, false };

        case SINK_MEMORY_AUTO_CAPTURE_LONG_INTERVAL_SEC:
            autoCaptureLongIntervalSecParameter.set(std::max(0.0f, value));
            return { nullptr, false };

        case SINK_MEMORY_AUTO_CAPTURE_MIN_VARIANCE:
            autoCaptureMinVarianceParameter.set(std::max(0.0f, value));
            return { nullptr, false };

        case SINK_MEMORY_AUTO_CAPTURE_MIN_ACTIVE_FRACTION:
            autoCaptureMinActiveFractionParameter.set(std::clamp(value, 0.0f, 1.0f));
            return { nullptr, false };

        case SINK_MEMORY_AUTO_CAPTURE_QUALITY_FLOOR:
            autoCaptureQualityFloorParameter.set(std::max(0.0f, value));
            return { nullptr, false };

        case SINK_MEMORY_AUTO_CAPTURE_ANCHOR_QUALITY_FLOOR:
            autoCaptureAnchorQualityFloorParameter.set(std::max(0.0f, value));
            return { nullptr, false };

        case SINK_MEMORY_AUTO_CAPTURE_LOW_QUALITY_RETRY_SEC:
            autoCaptureLowQualityRetrySecParameter.set(std::max(0.0f, value));
            return { nullptr, false };

        case SINK_MEMORY_AUTO_CAPTURE_ABS_IMPROVE:
            autoCaptureAbsImproveParameter.set(std::max(0.0f, value));
            return { nullptr, false };

        case SINK_MEMORY_AUTO_CAPTURE_REL_IMPROVE_RECENT:
            autoCaptureRelImproveRecentParameter.set(std::max(0.0f, value));
            return { nullptr, false };

        case SINK_MEMORY_AUTO_CAPTURE_REL_IMPROVE_MID:
            autoCaptureRelImproveMidParameter.set(std::max(0.0f, value));
            return { nullptr, false };

        case SINK_MEMORY_AUTO_CAPTURE_REL_IMPROVE_LONG:
            autoCaptureRelImproveLongParameter.set(std::max(0.0f, value));
            return { nullptr, false };

        default:
            return { nullptr, false };
    }
}

void MemoryBankController::applyIntent(const Intent& intent, float intentStrength) {
    // Chaos -> emit width (more chaos = wider/more random selection)
    float emitWidth = ofLerp(0.2f, 1.0f, intent.getChaos());
    emitWidthController.updateIntent(emitWidth, intentStrength);

    // Energy -> emit centre (high energy = recent memories)
    float emitCentre = ofLerp(0.3f, 0.9f, intent.getEnergy());
    emitCentreController.updateIntent(emitCentre, intentStrength);

    // Structure -> save width (more structure = more predictable/sequential saves)
    float saveWidth = ofLerp(0.5f, 0.0f, intent.getStructure());
    saveWidthController.updateIntent(saveWidth, intentStrength);
}

void MemoryBankController::update(const ofFbo& compositeFbo,
                                 const std::filesystem::path& configRootPath,
                                 float synthRunningTimeSec) {
    lastSynthRunningTimeSec = synthRunningTimeSec;

    int pendingSlot = memoryBank.processPendingSave(compositeFbo);
    if (pendingSlot >= 0) {
        noteManualSaveSlot(pendingSlot);
    }

    updateAutoCapture(compositeFbo, synthRunningTimeSec);

    if (memorySaveAllRequested) {
        memorySaveAllRequested = false;
        if (configRootPath.empty()) {
            ofLogWarning("MemoryBankController") << "Cannot save global memory bank: config root not set";
        } else {
            const std::filesystem::path folder = configRootPath / "memory" / "global";
            memoryBank.saveAllToFolder(folder);
        }
    }
}

static void initSteadyDueTimes(MemoryBankAutoCaptureState& state,
                              float nowSec,
                              float recentIntervalSec,
                              float midIntervalSec,
                              float longIntervalSec) {
    if (state.nextSlotDueTimeSec.size() != MemoryBank::NUM_SLOTS) {
        state.nextSlotDueTimeSec.assign(MemoryBank::NUM_SLOTS, 0.0f);
    }

    for (int slot = 0; slot < MemoryBank::NUM_SLOTS; ++slot) {
        float interval = getTimeBandIntervalSec(slot, recentIntervalSec, midIntervalSec, longIntervalSec);
        state.nextSlotDueTimeSec[slot] = nowSec + jitteredInterval(interval);
    }
}

static bool ensureAnalysisAllocated(MemoryBankAutoCaptureState& state, int analysisSize) {
    analysisSize = std::clamp(analysisSize, 8, 128);

    if (state.analysisSize == analysisSize && state.analysisFbo.isAllocated()) {
        return true;
    }

    state.analysisSize = analysisSize;
    state.analysisFbo.allocate(analysisSize, analysisSize, GL_RGB8);

    size_t pboSize = static_cast<size_t>(analysisSize) * static_cast<size_t>(analysisSize) * 3;
    state.pbo.allocate(pboSize, GL_STREAM_READ);
    state.pixels.assign(pboSize, 0);

    return state.analysisFbo.isAllocated();
}

static void beginAnalysis(MemoryBankAutoCaptureState& state,
                         const ofFbo& compositeFbo,
                         glm::vec2 cropTopLeft,
                         glm::vec2 cropSize) {
    if (!state.analysisFbo.isAllocated()) return;

    const int sz = state.analysisSize;

    state.analysisFbo.begin();
    ofClear(0, 0, 0, 255);
    ofPushStyle();
    ofEnableBlendMode(OF_BLENDMODE_DISABLED);
    ofSetColor(255);
    compositeFbo.getTexture().drawSubsection(0,
                                            0,
                                            static_cast<float>(sz),
                                            static_cast<float>(sz),
                                            cropTopLeft.x,
                                            cropTopLeft.y,
                                            cropSize.x,
                                            cropSize.y);
    ofPopStyle();
    state.analysisFbo.end();

    glBindFramebuffer(GL_READ_FRAMEBUFFER, state.analysisFbo.getId());
    glBindBuffer(GL_PIXEL_PACK_BUFFER, state.pbo.getId());
    glReadPixels(0, 0, sz, sz, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    if (state.fence) {
        glDeleteSync(state.fence);
        state.fence = nullptr;
    }

    state.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    state.framesWaited = 0;
    state.state = MemoryBankAutoCaptureState::State::PBO_WAITING;
}

static bool pollAnalysis(MemoryBankAutoCaptureState& state) {
    if (state.state != MemoryBankAutoCaptureState::State::PBO_WAITING || !state.fence) {
        return false;
    }

    state.framesWaited++;
    if (state.framesWaited < AUTO_CAPTURE_MIN_FRAMES_TO_WAIT) {
        return false;
    }

    GLenum result = glClientWaitSync(state.fence, 0, 0);
    if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED) {
        const int sz = state.analysisSize;
        size_t bytes = static_cast<size_t>(sz) * static_cast<size_t>(sz) * 3;

        if (state.pixels.size() != bytes) {
            state.pixels.assign(bytes, 0);
        }

        state.pbo.bind(GL_PIXEL_PACK_BUFFER);
        void* ptr = state.pbo.map(GL_READ_ONLY);
        if (ptr) {
            memcpy(state.pixels.data(), ptr, bytes);
            state.pbo.unmap();
        }
        state.pbo.unbind(GL_PIXEL_PACK_BUFFER);

        glDeleteSync(state.fence);
        state.fence = nullptr;
        state.state = MemoryBankAutoCaptureState::State::IDLE;
        return true;
    }

    if (result == GL_WAIT_FAILED || state.framesWaited > AUTO_CAPTURE_MAX_FRAMES_BEFORE_ABANDON) {
        glDeleteSync(state.fence);
        state.fence = nullptr;
        state.state = MemoryBankAutoCaptureState::State::IDLE;
    }

    return false;
}

static float getRelImproveForBand(int band,
                                 float relRecent,
                                 float relMid,
                                 float relLong) {
    switch (band) {
        case 0: return relLong;
        case 1: return relMid;
        default: return relRecent;
    }
}

static void tryLockAnchor(MemoryBankAutoCaptureState& state, float anchorQualityFloor) {
    if (state.lockedAnchorSlot >= 0) return;

    float bestTime = std::numeric_limits<float>::infinity();
    int bestSlot = -1;

    for (int slot = 0; slot <= 2; ++slot) {
        float q = state.slotQualityScore[slot];
        float t = state.slotCaptureTimeSec[slot];
        if (q >= anchorQualityFloor && t >= 0.0f) {
            if (t < bestTime) {
                bestTime = t;
                bestSlot = slot;
            }
        }
    }

    if (bestSlot >= 0) {
        state.lockedAnchorSlot = bestSlot;
        ofLogNotice("MemoryBankController") << "Locked long-band memory anchor: slot=" << bestSlot;
    }
}

bool MemoryBankController::getAutoCaptureSlotDebug(int slot,
                                                  float synthRunningTimeSec,
                                                  AutoCaptureSlotDebug& out) const {
    if (!autoCaptureState) return false;
    if (slot < 0 || slot >= MemoryBank::NUM_SLOTS) return false;

    const MemoryBankAutoCaptureState& state = *autoCaptureState;

    out.isOccupied = memoryBank.isOccupied(slot);
    out.band = getBandForSlot(slot);
    out.isAnchorLocked = (state.lockedAnchorSlot == slot);

    out.captureTimeSec = state.slotCaptureTimeSec[slot];
    out.variance = state.slotVariance[slot];
    out.activeFraction = state.slotActiveFraction[slot];
    out.qualityScore = state.slotQualityScore[slot];

    if (state.nextSlotDueTimeSec.size() == MemoryBank::NUM_SLOTS) {
        out.nextDueTimeSec = state.nextSlotDueTimeSec[slot];
    } else {
        out.nextDueTimeSec = -1.0f;
    }

    (void)synthRunningTimeSec;
    return true;
}

void MemoryBankController::updateAutoCapture(const ofFbo& compositeFbo, float synthRunningTimeSec) {
    if (!autoCaptureEnabledParameter) return;
    if (!compositeFbo.isAllocated()) return;

    if (!autoCaptureState) {
        autoCaptureState = std::make_unique<MemoryBankAutoCaptureState>();
        autoCaptureState->nextAttemptTimeSec = synthRunningTimeSec;
    }

    MemoryBankAutoCaptureState& state = *autoCaptureState;

    const int analysisSize = autoCaptureAnalysisSizeParameter;
    if (!ensureAnalysisAllocated(state, analysisSize)) {
        return;
    }

    const bool warmupFillMode = memoryBank.getFilledCount() < MemoryBank::NUM_SLOTS;

    // If analysis completed this frame, decide whether to save.
    if (pollAnalysis(state)) {
        // Active epsilon is the per-pixel luma deviation needed to count as "active".
        // Keep this fairly low so sparse-but-visible content still counts.
        constexpr float activeEpsilon = 0.02f;
        DensityMetrics metrics = computeDensityMetricsRgb(state.pixels, state.analysisSize, activeEpsilon);
        float newScore = computeQualityScore(metrics);

        float minVar = autoCaptureMinVarianceParameter;
        float minActive = autoCaptureMinActiveFractionParameter;

        // Warmup: start stricter, relax towards fill to ensure a populated bank.
        if (warmupFillMode) {
            float t = 0.0f;
            if (autoCaptureWarmupTargetSecParameter > 0.0f) {
                t = std::clamp(synthRunningTimeSec / autoCaptureWarmupTargetSecParameter.get(), 0.0f, 1.0f);
            }
            float scale = ofLerp(1.0f, 0.0f, t);
            minVar *= scale;
            minActive *= scale;
        }

        bool pass = (metrics.variance >= minVar) && (metrics.activeFraction >= minActive);

        bool savedAny = false;
        if (pass && !state.pendingSaveSlots.empty()) {
            for (int slot : state.pendingSaveSlots) {
                bool shouldSave = false;

                if (warmupFillMode) {
                    // Only fill empty slots in warmup.
                    shouldSave = !memoryBank.isOccupied(slot);
                } else {
                    float oldScore = state.slotQualityScore[slot];
                    int band = getBandForSlot(slot);
                    float relImprove = getRelImproveForBand(band,
                                                           autoCaptureRelImproveRecentParameter,
                                                           autoCaptureRelImproveMidParameter,
                                                           autoCaptureRelImproveLongParameter);

                    float absImprove = autoCaptureAbsImproveParameter;

                    if (oldScore < 0.0f) {
                        shouldSave = true;
                    } else {
                        shouldSave = (newScore > oldScore + absImprove) && (newScore > oldScore * (1.0f + relImprove));
                    }
                }

                if (!shouldSave) {
                    continue;
                }

                memoryBank.saveToSlotCrop(compositeFbo, slot, state.pendingCropTopLeft);

                state.slotCaptureTimeSec[slot] = synthRunningTimeSec;
                state.slotVariance[slot] = metrics.variance;
                state.slotActiveFraction[slot] = metrics.activeFraction;
                state.slotQualityScore[slot] = newScore;

                savedAny = true;

                if (!warmupFillMode && state.nextSlotDueTimeSec.size() == MemoryBank::NUM_SLOTS) {
                    float interval = getTimeBandIntervalSec(slot,
                                                           autoCaptureRecentIntervalSecParameter,
                                                           autoCaptureMidIntervalSecParameter,
                                                           autoCaptureLongIntervalSecParameter);
                    float due = synthRunningTimeSec + jitteredInterval(interval);

                    // If the slot is still below the quality floor, retry sooner.
                    if (newScore < autoCaptureQualityFloorParameter) {
                        due = std::min(due, synthRunningTimeSec + autoCaptureLowQualityRetrySecParameter);
                    }

                    state.nextSlotDueTimeSec[slot] = due;
                }
            }
        }

        state.pendingSaveSlots.clear();

        // Lock the long-band anchor once we have something good enough.
        tryLockAnchor(state, autoCaptureAnchorQualityFloorParameter);

        if (warmupFillMode) {
            state.nextAttemptTimeSec = synthRunningTimeSec + jitteredInterval(autoCaptureWarmupIntervalSecParameter, 0.25f);
        } else {
            // Next attempt aligns with next due slot.
            float nextDue = std::numeric_limits<float>::infinity();
            for (float t : state.nextSlotDueTimeSec) {
                nextDue = std::min(nextDue, t);
            }
            if (!std::isfinite(nextDue)) nextDue = synthRunningTimeSec + 1.0f;
            state.nextAttemptTimeSec = nextDue;

            // If we passed density, but didn't save (not meaningfully better), retry sooner.
            if (pass && !savedAny) {
                state.nextAttemptTimeSec = synthRunningTimeSec + jitteredInterval(autoCaptureRetryIntervalSecParameter, 0.25f);
            }
        }

        if (!pass) {
            state.nextAttemptTimeSec = synthRunningTimeSec + jitteredInterval(autoCaptureRetryIntervalSecParameter, 0.25f);
        }

        // If warmup just finished, initialize steady schedule.
        if (!warmupFillMode && state.nextSlotDueTimeSec.empty()) {
            initSteadyDueTimes(state,
                               synthRunningTimeSec,
                               autoCaptureRecentIntervalSecParameter,
                               autoCaptureMidIntervalSecParameter,
                               autoCaptureLongIntervalSecParameter);
        }

        return;
    }

    // If analysis is still pending, wait.
    if (state.state != MemoryBankAutoCaptureState::State::IDLE) {
        return;
    }

    // Not time to attempt yet.
    if (synthRunningTimeSec < state.nextAttemptTimeSec) {
        return;
    }

    state.pendingSaveSlots.clear();

    // Determine crop size used by memory saves.
    glm::vec2 cropSize = memoryBank.getMemorySize();

    // If memory size is larger than the composite (unlikely), just skip to avoid invalid crop math.
    if (cropSize.x > compositeFbo.getWidth() || cropSize.y > compositeFbo.getHeight()) {
        state.nextAttemptTimeSec = synthRunningTimeSec + jitteredInterval(autoCaptureRetryIntervalSecParameter, 0.25f);
        return;
    }

    if (warmupFillMode) {
        // Fill empty slots quickly. We fill lower indices first so that long-term slots
        // (0-2) naturally capture early performance content.
        int burst = std::clamp(autoCaptureWarmupBurstCountParameter.get(), 1, 4);
        for (int slot = 0; slot < MemoryBank::NUM_SLOTS && static_cast<int>(state.pendingSaveSlots.size()) < burst; ++slot) {
            if (!memoryBank.isOccupied(slot)) {
                state.pendingSaveSlots.push_back(slot);
            }
        }
    } else {
        if (state.nextSlotDueTimeSec.empty()) {
            initSteadyDueTimes(state,
                               synthRunningTimeSec,
                               autoCaptureRecentIntervalSecParameter,
                               autoCaptureMidIntervalSecParameter,
                               autoCaptureLongIntervalSecParameter);
        }

        float qualityFloor = autoCaptureQualityFloorParameter;

        // Find the next band that is due (or has low-quality slots that should be upgraded).
        std::array<float, 3> bandDue;
        bandDue.fill(std::numeric_limits<float>::infinity());

        for (int slot = 0; slot < MemoryBank::NUM_SLOTS; ++slot) {
            if (state.lockedAnchorSlot == slot) continue;

            float due = state.nextSlotDueTimeSec[slot];
            float q = state.slotQualityScore[slot];
            if (q >= 0.0f && q < qualityFloor) {
                due = std::min(due, synthRunningTimeSec);
            }

            int band = getBandForSlot(slot);
            bandDue[band] = std::min(bandDue[band], due);
        }

        int selectedBand = 0;
        float selectedBandDue = bandDue[0];
        for (int band = 1; band < 3; ++band) {
            if (bandDue[band] < selectedBandDue) {
                selectedBandDue = bandDue[band];
                selectedBand = band;
            }
        }

        if (!std::isfinite(selectedBandDue)) {
            state.nextAttemptTimeSec = synthRunningTimeSec + jitteredInterval(autoCaptureRetryIntervalSecParameter, 0.25f);
            return;
        }

        if (selectedBandDue > synthRunningTimeSec) {
            state.nextAttemptTimeSec = selectedBandDue;
            return;
        }

        // Within the band, target the lowest-quality slot (excluding the anchor).
        int bestSlot = -1;
        float bestQuality = std::numeric_limits<float>::infinity();

        for (int slot = 0; slot < MemoryBank::NUM_SLOTS; ++slot) {
            if (getBandForSlot(slot) != selectedBand) continue;
            if (state.lockedAnchorSlot == slot) continue;

            float q = state.slotQualityScore[slot];
            // Prefer unknown quality (e.g. loaded from disk or manual save) as low-quality.
            float effectiveQ = (q < 0.0f) ? -1.0f : q;

            if (bestSlot < 0 || effectiveQ < bestQuality) {
                bestSlot = slot;
                bestQuality = effectiveQ;
            }
        }

        if (bestSlot < 0) {
            state.nextAttemptTimeSec = synthRunningTimeSec + jitteredInterval(autoCaptureRetryIntervalSecParameter, 0.25f);
            return;
        }

        state.pendingSaveSlots.push_back(bestSlot);
    }

    if (state.pendingSaveSlots.empty()) {
        state.nextAttemptTimeSec = synthRunningTimeSec + jitteredInterval(autoCaptureRetryIntervalSecParameter, 0.25f);
        return;
    }

    float maxX = compositeFbo.getWidth() - cropSize.x;
    float maxY = compositeFbo.getHeight() - cropSize.y;

    state.pendingCropTopLeft = glm::vec2 { ofRandom(0, maxX), ofRandom(0, maxY) };
    state.pendingCropSize = cropSize;

    beginAnalysis(state, compositeFbo, state.pendingCropTopLeft, state.pendingCropSize);
}

bool MemoryBankController::loadGlobalMemories(const std::filesystem::path& configRootPath) {
    if (globalMemoryBankLoaded) {
        return false;
    }

    if (configRootPath.empty()) {
        ofLogWarning("MemoryBankController") << "Cannot load global memory bank: config root not set";
        return false;
    }

    const std::filesystem::path folder = configRootPath / "memory" / "global";
    memoryBank.loadAllFromFolder(folder);
    globalMemoryBankLoaded = true;
    return true;
}

void MemoryBankController::requestSaveAll() {
    memorySaveAllRequested = true;
}

std::map<std::string, int> MemoryBankController::getSinkNameIdMap() const {
    return {
        { "MemorySave", SINK_MEMORY_SAVE },
        { "MemorySaveSlot", SINK_MEMORY_SAVE_SLOT },
        { "MemoryEmit", SINK_MEMORY_EMIT },
        { "MemoryEmitSlot", SINK_MEMORY_EMIT_SLOT },
        { "MemoryEmitRandom", SINK_MEMORY_EMIT_RANDOM },
        { "MemoryEmitRandomNew", SINK_MEMORY_EMIT_RANDOM_NEW },
        { "MemoryEmitRandomOld", SINK_MEMORY_EMIT_RANDOM_OLD },
        { saveCentreParameter.getName(), SINK_MEMORY_SAVE_CENTRE },
        { saveWidthParameter.getName(), SINK_MEMORY_SAVE_WIDTH },
        { emitCentreParameter.getName(), SINK_MEMORY_EMIT_CENTRE },
        { emitWidthParameter.getName(), SINK_MEMORY_EMIT_WIDTH },
        { "MemoryClearAll", SINK_MEMORY_CLEAR_ALL },

        { autoCaptureEnabledParameter.getName(), SINK_MEMORY_AUTO_CAPTURE_ENABLED },
        { autoCaptureWarmupTargetSecParameter.getName(), SINK_MEMORY_AUTO_CAPTURE_WARMUP_TARGET_SEC },
        { autoCaptureWarmupIntervalSecParameter.getName(), SINK_MEMORY_AUTO_CAPTURE_WARMUP_INTERVAL_SEC },
        { autoCaptureRetryIntervalSecParameter.getName(), SINK_MEMORY_AUTO_CAPTURE_RETRY_INTERVAL_SEC },
        { autoCaptureRecentIntervalSecParameter.getName(), SINK_MEMORY_AUTO_CAPTURE_RECENT_INTERVAL_SEC },
        { autoCaptureMidIntervalSecParameter.getName(), SINK_MEMORY_AUTO_CAPTURE_MID_INTERVAL_SEC },
        { autoCaptureLongIntervalSecParameter.getName(), SINK_MEMORY_AUTO_CAPTURE_LONG_INTERVAL_SEC },
        { autoCaptureMinVarianceParameter.getName(), SINK_MEMORY_AUTO_CAPTURE_MIN_VARIANCE },
        { autoCaptureMinActiveFractionParameter.getName(), SINK_MEMORY_AUTO_CAPTURE_MIN_ACTIVE_FRACTION },

        { autoCaptureQualityFloorParameter.getName(), SINK_MEMORY_AUTO_CAPTURE_QUALITY_FLOOR },
        { autoCaptureAnchorQualityFloorParameter.getName(), SINK_MEMORY_AUTO_CAPTURE_ANCHOR_QUALITY_FLOOR },
        { autoCaptureLowQualityRetrySecParameter.getName(), SINK_MEMORY_AUTO_CAPTURE_LOW_QUALITY_RETRY_SEC },
        { autoCaptureAbsImproveParameter.getName(), SINK_MEMORY_AUTO_CAPTURE_ABS_IMPROVE },
        { autoCaptureRelImproveRecentParameter.getName(), SINK_MEMORY_AUTO_CAPTURE_REL_IMPROVE_RECENT },
        { autoCaptureRelImproveMidParameter.getName(), SINK_MEMORY_AUTO_CAPTURE_REL_IMPROVE_MID },
        { autoCaptureRelImproveLongParameter.getName(), SINK_MEMORY_AUTO_CAPTURE_REL_IMPROVE_LONG }
    };
}

} // namespace ofxMarkSynth
