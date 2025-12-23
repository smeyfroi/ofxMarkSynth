//
//  AsyncImageSaver.cpp
//  ofxMarkSynth
//
//  Created by Steve Meyfroidt on 23/12/2025.
//

#include "AsyncImageSaver.hpp"
#include "ofxTinyEXR.h"
#include "ofLog.h"
#include "ofGLUtils.h"

namespace ofxMarkSynth {

AsyncImageSaver::AsyncImageSaver(glm::vec2 imageSize_)
    : imageSize(imageSize_)
{
    size_t pboSize = static_cast<size_t>(imageSize.x)
                   * static_cast<size_t>(imageSize.y)
                   * 3 * sizeof(float);
    pbo.allocate(pboSize, GL_STREAM_READ);
}

AsyncImageSaver::~AsyncImageSaver() {
    flush();
}

bool AsyncImageSaver::requestSave(const ofFbo& sourceFbo, const std::string& filepath) {
    if (state != State::IDLE) {
        return false;  // Reject: PBO fetch already in progress
    }

    int w = static_cast<int>(imageSize.x);
    int h = static_cast<int>(imageSize.y);

    // Issue async read into PBO
    glBindFramebuffer(GL_READ_FRAMEBUFFER, sourceFbo.getId());
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo.getId());
    glReadPixels(0, 0, w, h, GL_RGB, GL_FLOAT, 0);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    // Create fence to track completion
    fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    pendingFilepath = filepath;
    state = State::PBO_WAITING;
    framesWaited = 0;

    return true;
}

void AsyncImageSaver::update() {
    pruneFinishedThreads();

    if (state == State::PBO_WAITING) {
        processPboTransfer();
    }
}

void AsyncImageSaver::processPboTransfer() {
    framesWaited++;

    // Don't check fence until minimum frames have passed
    if (framesWaited < FRAMES_TO_WAIT) {
        return;
    }

    // Non-blocking fence check
    GLenum result = glClientWaitSync(fence, 0, 0);

    if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED) {
        ofLogNotice("AsyncImageSaver") << "PBO transfer complete after " << framesWaited << " frames";
        completePboTransfer();

    } else if (result == GL_WAIT_FAILED || framesWaited > MAX_FRAMES_BEFORE_ABANDON) {
        ofLogError("AsyncImageSaver") << "PBO transfer failed/timed out after " << framesWaited << " frames";
        glDeleteSync(fence);
        fence = nullptr;
        state = State::IDLE;
        pendingFilepath.clear();
    }
    // else GL_TIMEOUT_EXPIRED - wait for next frame
}

void AsyncImageSaver::completePboTransfer() {
    int w = static_cast<int>(imageSize.x);
    int h = static_cast<int>(imageSize.y);
    size_t size = static_cast<size_t>(w) * static_cast<size_t>(h) * 3 * sizeof(float);

    ofFloatPixels pixels;
    pixels.allocate(w, h, OF_IMAGE_COLOR);

    pbo.bind(GL_PIXEL_PACK_BUFFER);
    void* ptr = pbo.map(GL_READ_ONLY);
    if (ptr) {
        memcpy(pixels.getData(), ptr, size);
        pbo.unmap();
    }
    pbo.unbind(GL_PIXEL_PACK_BUFFER);

    glDeleteSync(fence);
    fence = nullptr;

    startSaveThread(pendingFilepath, std::move(pixels));

    state = State::IDLE;
    pendingFilepath.clear();
}

void AsyncImageSaver::startSaveThread(const std::string& filepath, ofFloatPixels&& pixels) {
    auto thread = std::make_unique<SaveThread>();
    thread->save(filepath, std::move(pixels));
    threads.push_back(std::move(thread));
}

void AsyncImageSaver::pruneFinishedThreads() {
    threads.erase(
        std::remove_if(threads.begin(), threads.end(),
                       [](const std::unique_ptr<SaveThread>& t) {
                           return !t->isThreadRunning();
                       }),
        threads.end());
}

void AsyncImageSaver::flush() {
    // Complete any pending PBO transfer with blocking wait
    if (state == State::PBO_WAITING && fence) {
        ofLogNotice("AsyncImageSaver") << "Flush: waiting for PBO transfer";
        glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
        completePboTransfer();
    }

    // Wait for all I/O threads
    for (auto& t : threads) {
        if (t->isThreadRunning()) {
            ofLogNotice("AsyncImageSaver") << "Flush: waiting for save thread";
            t->waitForThread(false);
        }
    }
    threads.clear();
}

int AsyncImageSaver::getActiveSaveCount() const {
    int count = static_cast<int>(threads.size());
    if (state == State::PBO_WAITING) {
        count++;
    }
    return count;
}

// SaveThread implementation
void AsyncImageSaver::SaveThread::save(const std::string& filepath_, ofFloatPixels&& pixels_) {
    filepath = filepath_;
    pixels = std::move(pixels_);
    startThread();
}

void AsyncImageSaver::SaveThread::threadedFunction() {
    ofLogNotice("AsyncImageSaver") << "Saving to " << filepath;

    ofxTinyEXR exrIO;
    bool saved = exrIO.savePixels(pixels, filepath);
    if (!saved) {
        ofLogError("AsyncImageSaver") << "Failed to save EXR: " << filepath;
    }

    ofLogNotice("AsyncImageSaver") << "Done saving " << filepath;
}

} // namespace ofxMarkSynth
