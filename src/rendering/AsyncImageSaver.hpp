//
//  AsyncImageSaver.hpp
//  ofxMarkSynth
//
//  Created by Steve Meyfroidt on 23/12/2025.
//

#pragma once

#include "ofThread.h"
#include "ofPixels.h"
#include "ofBufferObject.h"
#include "ofFbo.h"
#include <string>
#include <vector>
#include <memory>

namespace ofxMarkSynth {

/// Handles async image saving with PBO-based GPU readback.
/// 
/// Usage:
///   - Call requestSave() to initiate a save (rejects if PBO fetch in progress)
///   - Call update() once per frame from draw()
///   - Call flush() on shutdown to ensure all saves complete
///   - Call getActiveSaveCount() for status display
///
/// The implementation uses a 5-frame delay before mapping the PBO to ensure
/// the GPU DMA transfer completes without stalling the render pipeline.
class AsyncImageSaver {
public:
    AsyncImageSaver(glm::vec2 imageSize);
    ~AsyncImageSaver();

    /// Main thread: call once per frame from draw()
    void update();

    /// Main thread: request a save. Returns true if accepted, false if a PBO
    /// fetch is already in progress.
    bool requestSave(const ofFbo& sourceFbo, const std::string& filepath);

    /// Main thread: force completion of any pending work (for shutdown)
    void flush();

    /// Count of active operations (PBO wait + I/O threads)
    int getActiveSaveCount() const;

private:
    static constexpr int FRAMES_TO_WAIT = 5;
    static constexpr int MAX_FRAMES_BEFORE_ABANDON = 60;

    // PBO state (main thread only)
    ofBufferObject pbo;
    glm::vec2 imageSize;

    enum class State { IDLE, PBO_WAITING };
    State state { State::IDLE };
    int framesWaited { 0 };
    GLsync fence { nullptr };
    std::string pendingFilepath;

    // I/O thread (inner class)
    struct SaveThread : public ofThread {
        void save(const std::string& filepath, ofFloatPixels&& pixels);
        void threadedFunction() override;
        std::string filepath;
        ofFloatPixels pixels;
    };
    std::vector<std::unique_ptr<SaveThread>> threads;

    void pruneFinishedThreads();
    void processPboTransfer();
    
    /// Maps the PBO, copies pixels, cleans up fence, and starts a save thread.
    /// Called when the fence is signaled (or during flush with blocking wait).
    void completePboTransfer();
    
    void startSaveThread(const std::string& filepath, ofFloatPixels&& pixels);
};

} // namespace ofxMarkSynth
