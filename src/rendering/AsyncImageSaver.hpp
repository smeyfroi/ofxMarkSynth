//  AsyncImageSaver.hpp
//  ofxMarkSynth
//
//  Created by Steve Meyfroidt on 23/12/2025.
//

#pragma once

#include "ofBufferObject.h"
#include "ofFbo.h"
#include "ofThread.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ofxMarkSynth {

/// Handles async image saving with PBO-based GPU readback.
///
/// Usage:
///   - Call requestSave() to initiate a save (rejects if a save is in progress)
///   - Call update() once per frame from draw()
///   - Call flush() on shutdown to ensure all saves complete
///   - Call getActiveSaveCount() for status display
///
/// The implementation uses a short delay before mapping the PBO to ensure the GPU
/// DMA transfer completes without stalling the render pipeline.
class AsyncImageSaver {
public:
    AsyncImageSaver(glm::vec2 imageSize);
    ~AsyncImageSaver();

    /// Main thread: call once per frame from draw()
    void update();

    /// Main thread: request a save. Returns true if accepted, false if a save is already in progress.
    bool requestSave(const ofFbo& sourceFbo, const std::string& filepath);

    /// Main thread: force completion of any pending work (for shutdown)
    void flush();

    /// Count of active operations (PBO wait + I/O threads)
    int getActiveSaveCount() const;

    /// Convenience wrapper for full-res autosave scheduling.
    ///
    /// Policy:
    /// - Uses the caller-provided timebase (usually clock time)
    /// - Requires saver to be fully idle (no overlap)
    /// - Maintains an internal due-time with jitter
    ///
    /// Returns true if an autosave was started this call.
    bool requestAutoSaveIfDue(const ofFbo& sourceFbo,
                              float timeSec,
                              float intervalSec,
                              float jitterSec,
                              const std::function<std::string()>& filepathFactory);

private:
    // PBO state (main thread only)
    ofBufferObject pbo;
    glm::vec2 imageSize;

    enum class State {
        IDLE,
        PBO_WAITING
    };
    State state { State::IDLE };
    int framesWaited { 0 };
    GLsync fence { nullptr };
    std::string pendingFilepath;

    float nextAutoSnapshotDueConfigTimeSec { -1.0f };

    // I/O thread (inner class)
    struct SaveThread : public ofThread {
        void save(const std::string& filepath,
                  int width,
                  int height,
                  std::unique_ptr<std::uint16_t[]>&& interleavedRgb,
                  std::size_t pixelCount);
        void threadedFunction() override;

        std::string filepath;
        int width { 0 };
        int height { 0 };
        std::unique_ptr<std::uint16_t[]> interleavedRgb;
        std::size_t pixelCount { 0 };
    };
    std::vector<std::unique_ptr<SaveThread>> threads;

    void pruneFinishedThreads();
    void processPboTransfer();
    void completePboTransfer();

    void startSaveThread(const std::string& filepath,
                         int width,
                         int height,
                         std::unique_ptr<std::uint16_t[]>&& interleavedRgb,
                         std::size_t pixelCount);

    std::size_t getImageByteSize() const;
};

} // namespace ofxMarkSynth
