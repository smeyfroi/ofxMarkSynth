//
//  VideoRecorder.hpp
//  ofxMarkSynth
//
//  Handles video recording with async PBO-based pixel readback.
//  macOS only (uses ofxFFmpegRecorder with VideoToolbox).
//

#pragma once

#include "ofConstants.h"  // For TARGET_MAC definition

#ifdef TARGET_MAC

#include "ofxFFmpegRecorder.h"
#include "ofFbo.h"
#include "ofBufferObject.h"
#include "ofPixels.h"
#include <filesystem>
#include <functional>
#include <glm/vec2.hpp>



namespace ofxMarkSynth {



/// Handles video recording with async PBO-based pixel readback.
/// macOS only (uses ofxFFmpegRecorder with VideoToolbox).
class VideoRecorder {
public:
    VideoRecorder() = default;
    
    /// Initialize recorder resources
    void setup(glm::vec2 compositeSize, const std::filesystem::path& ffmpegPath);
    
    /// Start recording to the specified path
    void startRecording(const std::string& outputPath);
    
    /// Stop recording, flushing any pending frames
    void stopRecording();
    
    /// Shutdown and cleanup (call on app exit)
    void shutdown();
    
    /// Capture a frame. Call during draw() when recording.
    /// @param renderCallback Called with the recorder FBO to render content into
    void captureFrame(std::function<void(ofFbo& fbo)> renderCallback);
    
    /// Check if currently recording
    bool isRecording() const;
    
    /// Get recorder FBO size (for computing render scale)
    glm::vec2 getSize() const { return compositeSize_; }

private:
    static constexpr int NUM_PBOS { 2 };
    
    ofxFFmpegRecorder recorder_;
    glm::vec2 compositeSize_;
    ofFbo compositeFbo_;
    
    ofBufferObject pbos_[NUM_PBOS];
    int pboWriteIndex_ { 0 };
    int frameCount_ { 0 };
    ofPixels pixels_;
    
    bool isSetup_ { false };
    
    /// Flush the last pending frame from PBO before stopping
    void flushPendingFrame();
};



} // namespace ofxMarkSynth

#endif // TARGET_MAC
