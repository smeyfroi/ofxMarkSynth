//  AsyncImageSaver.cpp
//  ofxMarkSynth
//
//  Created by Steve Meyfroidt on 23/12/2025.
//

#include "rendering/AsyncImageSaver.hpp"
#include "rendering/RenderingConstants.h"
#include "ofGLUtils.h"
#include "ofLog.h"
#include "ofUtils.h"
#include "tinyexr.h"
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <new>

#ifdef __APPLE__
#include <pthread.h>
#include <pthread/qos.h>
#endif

namespace ofxMarkSynth {

static bool saveHalfRgbExrUncompressed(std::unique_ptr<std::uint16_t[]>&& interleavedRgb,
                                      std::size_t pixelCount,
                                      int width,
                                      int height,
                                      const std::string& filepath) {
    if (width <= 0 || height <= 0) {
        return false;
    }

    if (pixelCount != static_cast<std::size_t>(width) * static_cast<std::size_t>(height)) {
        return false;
    }

    if (!interleavedRgb) {
        return false;
    }

    // TinyEXR expects planar data and channels in (A)BGR order.
    std::vector<std::uint16_t> images[3];
    images[0].resize(pixelCount);  // B
    images[1].resize(pixelCount);  // G
    images[2].resize(pixelCount);  // R

    for (std::size_t i = 0; i < pixelCount; ++i) {
        images[2][i] = interleavedRgb[3 * i + 0];
        images[1][i] = interleavedRgb[3 * i + 1];
        images[0][i] = interleavedRgb[3 * i + 2];
    }

    // Release the (large) interleaved buffer early to reduce peak memory usage.
    interleavedRgb.reset();

    EXRHeader header;
    InitEXRHeader(&header);
    header.compression_type = TINYEXR_COMPRESSIONTYPE_NONE;

    header.num_channels = 3;
    header.channels = static_cast<EXRChannelInfo*>(malloc(sizeof(EXRChannelInfo) * 3));
    if (!header.channels) {
        return false;
    }

    std::strncpy(header.channels[0].name, "B", 255);
    std::strncpy(header.channels[1].name, "G", 255);
    std::strncpy(header.channels[2].name, "R", 255);
    header.channels[0].name[1] = '\0';
    header.channels[1].name[1] = '\0';
    header.channels[2].name[1] = '\0';

    header.pixel_types = static_cast<int*>(malloc(sizeof(int) * 3));
    header.requested_pixel_types = static_cast<int*>(malloc(sizeof(int) * 3));
    if (!header.pixel_types || !header.requested_pixel_types) {
        free(header.channels);
        if (header.pixel_types) free(header.pixel_types);
        if (header.requested_pixel_types) free(header.requested_pixel_types);
        return false;
    }

    for (int i = 0; i < 3; ++i) {
        header.pixel_types[i] = TINYEXR_PIXELTYPE_HALF;
        header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF;
    }

    EXRImage image;
    InitEXRImage(&image);
    image.num_channels = 3;
    image.width = width;
    image.height = height;

    unsigned char* image_ptr[3] = {
        reinterpret_cast<unsigned char*>(images[0].data()),
        reinterpret_cast<unsigned char*>(images[1].data()),
        reinterpret_cast<unsigned char*>(images[2].data())
    };

    image.images = image_ptr;

    const char* err = nullptr;
    int ret = SaveEXRImageToFile(&image, &header, filepath.c_str(), &err);

    free(header.channels);
    free(header.pixel_types);
    free(header.requested_pixel_types);

    if (ret != TINYEXR_SUCCESS) {
        if (err) {
            ofLogError("AsyncImageSaver") << "TinyEXR save failed: " << err;
            FreeEXRErrorMessage(err);
        } else {
            ofLogError("AsyncImageSaver") << "TinyEXR save failed: unknown error";
        }
        return false;
    }

    return true;
}

AsyncImageSaver::AsyncImageSaver(glm::vec2 imageSize_)
    : imageSize(imageSize_) {
    pbo.allocate(getImageByteSize(), GL_STREAM_READ);
}

AsyncImageSaver::~AsyncImageSaver() {
    flush();
}

bool AsyncImageSaver::requestAutoSaveIfDue(const ofFbo& sourceFbo,
                                          float timeSec,
                                          float intervalSec,
                                          float jitterSec,
                                          const std::function<std::string()>& filepathFactory) {
    if (intervalSec <= 0.0f) {
        return false;
    }

    // No overlap.
    if (getActiveSaveCount() != 0) {
        return false;
    }

    // Initialize schedule on first eligible frame.
    if (nextAutoSnapshotDueConfigTimeSec < 0.0f) {
        float jitter = ofRandom(-jitterSec, jitterSec);
        nextAutoSnapshotDueConfigTimeSec = std::max(intervalSec, intervalSec + jitter);
    }

    if (timeSec < nextAutoSnapshotDueConfigTimeSec) {
        return false;
    }

    std::string filepath = filepathFactory();
    if (filepath.empty()) {
        return false;
    }

    bool accepted = requestSave(sourceFbo, filepath);
    if (!accepted) {
        return false;
    }

    // Next due time with jitter, ensuring positive interval.
    float jitter = ofRandom(-jitterSec, jitterSec);
    float minDelta = std::max(1.0f, intervalSec - jitterSec);
    float delta = std::max(minDelta, intervalSec + jitter);
    nextAutoSnapshotDueConfigTimeSec = timeSec + delta;

    return true;
}

std::size_t AsyncImageSaver::getImageByteSize() const {
    int w = static_cast<int>(imageSize.x);
    int h = static_cast<int>(imageSize.y);
    return static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 3 * sizeof(std::uint16_t);
}

bool AsyncImageSaver::requestSave(const ofFbo& sourceFbo, const std::string& filepath) {
    if (state != State::IDLE) {
        return false;
    }

    int w = static_cast<int>(imageSize.x);
    int h = static_cast<int>(imageSize.y);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, sourceFbo.getId());
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo.getId());
    glReadPixels(0, 0, w, h, GL_RGB, GL_HALF_FLOAT, 0);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

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

    if (framesWaited < PBO_FRAMES_TO_WAIT) {
        return;
    }

    if (!fence) {
        ofLogError("AsyncImageSaver") << "PBO_WAITING without fence";
        state = State::IDLE;
        pendingFilepath.clear();
        return;
    }

    GLenum result = glClientWaitSync(fence, 0, 0);

    if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED) {
        completePboTransfer();

    } else if (result == GL_WAIT_FAILED || framesWaited > PBO_MAX_FRAMES_BEFORE_ABANDON) {
        ofLogError("AsyncImageSaver") << "PBO transfer failed/timed out after " << framesWaited << " frames";
        glDeleteSync(fence);
        fence = nullptr;
        state = State::IDLE;
        pendingFilepath.clear();
    }
}

void AsyncImageSaver::completePboTransfer() {
    if (!fence) {
        state = State::IDLE;
        pendingFilepath.clear();
        return;
    }

    // Fence is already signaled; safe to delete now.
    glDeleteSync(fence);
    fence = nullptr;

    const std::size_t byteSize = getImageByteSize();
    const std::size_t elementCount = byteSize / sizeof(std::uint16_t);

    int w = static_cast<int>(imageSize.x);
    int h = static_cast<int>(imageSize.y);
    std::size_t pixelCount = static_cast<std::size_t>(w) * static_cast<std::size_t>(h);

    pbo.bind(GL_PIXEL_PACK_BUFFER);
    void* pboPtr = pbo.map(GL_READ_ONLY);
    pbo.unbind(GL_PIXEL_PACK_BUFFER);

    if (!pboPtr) {
        ofLogError("AsyncImageSaver") << "Failed to map PBO";
        state = State::IDLE;
        pendingFilepath.clear();
        return;
    }

    auto cpu = std::unique_ptr<std::uint16_t[]>(new (std::nothrow) std::uint16_t[elementCount]);
    if (!cpu) {
        ofLogError("AsyncImageSaver") << "Failed to allocate CPU image buffer";
        pbo.bind(GL_PIXEL_PACK_BUFFER);
        pbo.unmap();
        pbo.unbind(GL_PIXEL_PACK_BUFFER);
        state = State::IDLE;
        pendingFilepath.clear();
        return;
    }

    std::memcpy(cpu.get(), pboPtr, byteSize);

    pbo.bind(GL_PIXEL_PACK_BUFFER);
    pbo.unmap();
    pbo.unbind(GL_PIXEL_PACK_BUFFER);

    startSaveThread(pendingFilepath, w, h, std::move(cpu), pixelCount);

    pendingFilepath.clear();
    state = State::IDLE;
}

void AsyncImageSaver::startSaveThread(const std::string& filepath,
                                     int width,
                                     int height,
                                     std::unique_ptr<std::uint16_t[]>&& interleavedRgb,
                                     std::size_t pixelCount) {
    auto thread = std::make_unique<SaveThread>();
    thread->save(filepath, width, height, std::move(interleavedRgb), pixelCount);
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
    // Complete any pending PBO transfer with blocking wait.
    if (state == State::PBO_WAITING && fence) {
        ofLogNotice("AsyncImageSaver") << "Flush: waiting for PBO transfer";
        glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
        completePboTransfer();
    }

    // Wait for all I/O threads.
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
    if (state != State::IDLE) {
        count++;
    }
    return count;
}

// SaveThread implementation
void AsyncImageSaver::SaveThread::save(const std::string& filepath_,
                                      int width_,
                                      int height_,
                                      std::unique_ptr<std::uint16_t[]>&& interleavedRgb_,
                                      std::size_t pixelCount_) {
    filepath = filepath_;
    width = width_;
    height = height_;
    interleavedRgb = std::move(interleavedRgb_);
    pixelCount = pixelCount_;
    startThread();
}

void AsyncImageSaver::SaveThread::threadedFunction() {
#ifdef __APPLE__
    // Keep EXR encoding + disk I/O from starving the render thread.
    pthread_set_qos_class_self_np(QOS_CLASS_BACKGROUND, 0);
#endif

    ofLogNotice("AsyncImageSaver") << "Saving to " << filepath;

    bool saved = saveHalfRgbExrUncompressed(std::move(interleavedRgb), pixelCount, width, height, filepath);
    if (!saved) {
        ofLogError("AsyncImageSaver") << "Failed to save EXR: " << filepath;
    }

    ofLogNotice("AsyncImageSaver") << "Done saving " << filepath;
}

} // namespace ofxMarkSynth
