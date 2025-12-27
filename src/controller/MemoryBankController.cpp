//
//  MemoryBankController.cpp
//  ofxMarkSynth
//
//  Controller wrapping MemoryBank with parameters, sink handling, and intent application.
//

#include "controller/MemoryBankController.hpp"
#include "core/Intent.hpp"
#include "ofUtils.h"
#include "ofLog.h"
#include "ofMath.h"

namespace ofxMarkSynth {

MemoryBankController::MemoryBankController() = default;

void MemoryBankController::allocate(glm::vec2 memorySize) {
    memoryBank.allocate(memorySize, GL_RGBA8);
}

void MemoryBankController::buildParameterGroup() {
    parameters.clear();
    parameters.setName("MemoryBank");
    parameters.add(saveCentreParameter);
    parameters.add(saveWidthParameter);
    parameters.add(emitCentreParameter);
    parameters.add(emitWidthParameter);
    parameters.add(emitMinIntervalParameter);
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

MemoryBankController::EmitResult MemoryBankController::handleSink(int sinkId, float value, const ofFbo& compositeFbo, float agency) {
    switch (sinkId) {
        // Memory bank: save operations
        case SINK_MEMORY_SAVE:
            if (value > 0.5f) {
                memoryBank.save(compositeFbo,
                    saveCentreController.value,
                    saveWidthController.value);
            }
            return { nullptr, false };

        case SINK_MEMORY_SAVE_SLOT:
            {
                int slot = static_cast<int>(value) % MemoryBank::NUM_SLOTS;
                memoryBank.saveToSlot(compositeFbo, slot);
            }
            return { nullptr, false };

        // Memory bank: emit operations
        case SINK_MEMORY_EMIT:
            if (value > 0.5f) {
                return emitWithRateLimit(
                    memoryBank.select(emitCentreController.value,
                                      emitWidthController.value));
            }
            return { nullptr, false };

        case SINK_MEMORY_EMIT_SLOT:
            {
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
                return emitWithRateLimit(
                    memoryBank.selectWeightedRecent(emitCentreController.value,
                                                    emitWidthController.value));
            }
            return { nullptr, false };

        case SINK_MEMORY_EMIT_RANDOM_OLD:
            if (value > 0.5f) {
                return emitWithRateLimit(
                    memoryBank.selectWeightedOld(emitCentreController.value,
                                                 emitWidthController.value));
            }
            return { nullptr, false };

        // Memory bank: parameter updates
        case SINK_MEMORY_SAVE_CENTRE:
            saveCentreController.updateAuto(value, agency);
            return { nullptr, false };

        case SINK_MEMORY_SAVE_WIDTH:
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
                ofLogNotice("MemoryBankController") << "Memory bank cleared";
            }
            return { nullptr, false };

        default:
            // Not a memory sink - caller should handle
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

void MemoryBankController::update(const ofFbo& compositeFbo, const std::filesystem::path& configRootPath) {
    // Process any deferred memory bank saves (requested from GUI)
    memoryBank.processPendingSave(compositeFbo);

    // Process save-all request
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

bool MemoryBankController::loadGlobalMemories(const std::filesystem::path& configRootPath) {
    if (globalMemoryBankLoaded) {
        return false; // Already loaded
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
        { "MemoryClearAll", SINK_MEMORY_CLEAR_ALL }
    };
}

} // namespace ofxMarkSynth
