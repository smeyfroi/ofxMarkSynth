//
//  VideoRecorder.cpp
//  ofxMarkSynth
//
//  Handles video recording with async PBO-based pixel readback.
//

#include "rendering/VideoRecorder.hpp"
#include "rendering/RenderingConstants.h"

#ifdef TARGET_MAC

#include "ofLog.h"
#include "ofGraphics.h"



namespace ofxMarkSynth {



void VideoRecorder::setup(glm::vec2 compositeSize, const std::filesystem::path& ffmpegPath) {
    compositeSize_ = compositeSize;
    
    compositeFbo_.allocate(compositeSize_.x, compositeSize_.y, GL_RGB);
    
    recorder_.setup(/*video*/true, /*audio*/false, compositeFbo_.getSize(),
                    DEFAULT_VIDEO_FPS, DEFAULT_VIDEO_BITRATE);
    recorder_.setOverWrite(true);
    recorder_.setFFmpegPath(ffmpegPath.string());
    recorder_.setVideoCodec(DEFAULT_VIDEO_CODEC);
    
    // Allocate PBOs for async pixel readback
    size_t pboSize = compositeSize_.x * compositeSize_.y * 3;  // RGB bytes
    for (int i = 0; i < NUM_PBOS; i++) {
        pbos_[i].allocate(pboSize, GL_DYNAMIC_READ);
    }
    pixels_.allocate(compositeSize_.x, compositeSize_.y, OF_PIXELS_RGB);
    
    isSetup_ = true;
    ofLogNotice("VideoRecorder") << "Setup complete: " << compositeSize_.x << "x" << compositeSize_.y;
}

void VideoRecorder::startRecording(const std::string& outputPath) {
    if (!isSetup_) {
        ofLogError("VideoRecorder") << "Cannot start recording: not setup";
        return;
    }
    
    if (recorder_.isRecording()) {
        ofLogWarning("VideoRecorder") << "Already recording";
        return;
    }
    
    recorder_.setOutputPath(outputPath);
    
    // Reset PBO state for new recording
    pboWriteIndex_ = 0;
    frameCount_ = 0;
    
    recorder_.startCustomRecord();
    ofLogNotice("VideoRecorder") << "Started recording to: " << outputPath;
}

void VideoRecorder::stopRecording() {
    if (!recorder_.isRecording()) return;
    
    flushPendingFrame();
    recorder_.stop();
    ofLogNotice("VideoRecorder") << "Stopped recording";
}

void VideoRecorder::shutdown() {
    if (recorder_.isRecording()) {
        ofLogNotice("VideoRecorder") << "Stopping recording on shutdown";
        stopRecording();
    }
}

void VideoRecorder::captureFrame(std::function<void(ofFbo& fbo)> renderCallback) {
    if (!recorder_.isRecording()) return;
    
    // Render content into recorder FBO
    compositeFbo_.begin();
    renderCallback(compositeFbo_);
    compositeFbo_.end();
    
    int width = compositeFbo_.getWidth();
    int height = compositeFbo_.getHeight();
    
    // Bind FBO for reading
    compositeFbo_.bind();
    
    // Start async read into current PBO (non-blocking)
    pbos_[pboWriteIndex_].bind(GL_PIXEL_PACK_BUFFER);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, 0);
    pbos_[pboWriteIndex_].unbind(GL_PIXEL_PACK_BUFFER);
    
    compositeFbo_.unbind();
    
    // Read from previous PBO (should be ready now) - but only after first frame
    if (frameCount_ > 0) {
        int readIndex = (pboWriteIndex_ + 1) % NUM_PBOS;
        pbos_[readIndex].bind(GL_PIXEL_PACK_BUFFER);
        void* ptr = pbos_[readIndex].map(GL_READ_ONLY);
        if (ptr) {
            memcpy(pixels_.getData(), ptr, width * height * 3);
            pbos_[readIndex].unmap();
        }
        pbos_[readIndex].unbind(GL_PIXEL_PACK_BUFFER);
        recorder_.addFrame(pixels_);
    }
    
    pboWriteIndex_ = (pboWriteIndex_ + 1) % NUM_PBOS;
    frameCount_++;
}

void VideoRecorder::flushPendingFrame() {
    if (frameCount_ == 0) return;
    
    int width = compositeFbo_.getWidth();
    int height = compositeFbo_.getHeight();
    
    int readIndex = (pboWriteIndex_ + NUM_PBOS - 1) % NUM_PBOS;
    pbos_[readIndex].bind(GL_PIXEL_PACK_BUFFER);
    void* ptr = pbos_[readIndex].map(GL_READ_ONLY);
    if (ptr) {
        memcpy(pixels_.getData(), ptr, width * height * 3);
        pbos_[readIndex].unmap();
        recorder_.addFrame(pixels_);
    }
    pbos_[readIndex].unbind(GL_PIXEL_PACK_BUFFER);
}

bool VideoRecorder::isRecording() const {
    return recorder_.isRecording();
}



} // namespace ofxMarkSynth

#endif // TARGET_MAC
