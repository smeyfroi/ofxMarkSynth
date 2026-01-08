//
//  MemoryBank.cpp
//  ofxMarkSynth
//
//  Memory bank for storing texture fragments captured from the composite
//  during live performance, enabling recall of earlier visual states.
//

#include "core/MemoryBank.hpp"

#include "ofGraphics.h"
#include "ofImage.h"
#include "ofLog.h"
#include "ofMath.h"

#include <algorithm>
#include <cmath>



namespace ofxMarkSynth {



static std::filesystem::path getSlotFilePath(const std::filesystem::path& folder, int slot) {
    return folder / ("slot-" + std::to_string(slot) + ".png");
}

static void removeSlotFromOrder(std::vector<int>& order, int slot) {
    order.erase(std::remove(order.begin(), order.end(), slot), order.end());
}

static void updateOrderMostRecent(std::vector<int>& order, int slot) {
    removeSlotFromOrder(order, slot);
    order.push_back(slot);
}

static int findFirstFreeSlot(const std::array<bool, MemoryBank::NUM_SLOTS>& occupied) {
    for (int i = 0; i < MemoryBank::NUM_SLOTS; ++i) {
        if (!occupied[i]) return i;
    }
    return -1;
}

static bool isValidSlotIndex(int slot) {
    return slot >= 0 && slot < MemoryBank::NUM_SLOTS;
}

static ofPixels ensureRgbaPixels(const ofPixels& in) {
    if (in.getNumChannels() == 4) {
        return in;
    }

    const int w = in.getWidth();
    const int h = in.getHeight();

    ofPixels out;
    out.allocate(w, h, OF_PIXELS_RGBA);

    const int inChannels = in.getNumChannels();
    const unsigned char* src = in.getData();
    unsigned char* dst = out.getData();

    const size_t n = static_cast<size_t>(w) * static_cast<size_t>(h);
    for (size_t i = 0; i < n; ++i) {
        unsigned char r = 0;
        unsigned char g = 0;
        unsigned char b = 0;
        unsigned char a = 255;

        if (inChannels == 3) {
            r = src[i * 3 + 0];
            g = src[i * 3 + 1];
            b = src[i * 3 + 2];
        } else if (inChannels == 1) {
            r = g = b = src[i];
        } else if (inChannels > 0) {
            // Fallback: copy first channel into RGB.
            r = g = b = src[i * inChannels];
        }

        dst[i * 4 + 0] = r;
        dst[i * 4 + 1] = g;
        dst[i * 4 + 2] = b;
        dst[i * 4 + 3] = a;
    }

    return out;
}



void MemoryBank::allocate(glm::vec2 size, GLint internalFormat) {
    memorySize = size;

    for (int i = 0; i < NUM_SLOTS; i++) {
        slots[i].allocate(memorySize.x, memorySize.y, internalFormat);
        slots[i].begin();
        ofClear(0, 0, 0, 0);
        slots[i].end();
    }

    allocated = true;
    occupied.fill(false);
    occupiedCount = 0;
    saveOrder.clear();
    pendingSaveSlot = -1;
}

int MemoryBank::save(const ofFbo& source, float centre, float width) {
    if (!allocated) {
        ofLogError("MemoryBank") << "Cannot save: not allocated";
        return -1;
    }

    const int existingCount = static_cast<int>(saveOrder.size());
    const bool hasFreeSlot = occupiedCount < NUM_SLOTS;
    const int maxIndex = (hasFreeSlot) ? existingCount : std::max(0, existingCount - 1);

    int selectedIndex = selectSlotIndex(centre, width, maxIndex);

    int slot = -1;
    if (hasFreeSlot && selectedIndex == existingCount) {
        slot = findFirstFreeSlot(occupied);
    } else {
        if (selectedIndex < 0 || selectedIndex >= existingCount) {
            selectedIndex = std::clamp(selectedIndex, 0, std::max(0, existingCount - 1));
        }
        if (!saveOrder.empty()) {
            slot = saveOrder[selectedIndex];
        } else {
            slot = findFirstFreeSlot(occupied);
        }
    }

    if (!isValidSlotIndex(slot)) {
        ofLogError("MemoryBank") << "Cannot save: no free slot available";
        return -1;
    }

    saveToSlot(source, slot);
    return slot;
}

void MemoryBank::saveToSlot(const ofFbo& source, int slot) {
    if (!allocated) {
        ofLogError("MemoryBank") << "Cannot save: not allocated";
        return;
    }
    if (!isValidSlotIndex(slot)) {
        ofLogError("MemoryBank") << "Invalid slot index: " << slot;
        return;
    }

    captureRandomCrop(slots[slot], source);

    if (!occupied[slot]) {
        occupied[slot] = true;
        occupiedCount++;
    }

    updateOrderMostRecent(saveOrder, slot);
}

void MemoryBank::saveToSlotCrop(const ofFbo& source, int slot, glm::vec2 cropTopLeft) {
    if (!allocated) {
        ofLogError("MemoryBank") << "Cannot save: not allocated";
        return;
    }
    if (!isValidSlotIndex(slot)) {
        ofLogError("MemoryBank") << "Invalid slot index: " << slot;
        return;
    }

    captureCrop(slots[slot], source, cropTopLeft);

    if (!occupied[slot]) {
        occupied[slot] = true;
        occupiedCount++;
    }

    updateOrderMostRecent(saveOrder, slot);
}

int MemoryBank::processPendingSave(const ofFbo& source) {
    if (pendingSaveSlot >= 0) {
        int slot = pendingSaveSlot;
        saveToSlot(source, slot);
        pendingSaveSlot = -1;
        return slot;
    }
    return -1;
}

void MemoryBank::captureRandomCrop(ofFbo& dest, const ofFbo& source) {
    float sourceW = source.getWidth();
    float sourceH = source.getHeight();
    float destW = dest.getWidth();
    float destH = dest.getHeight();

    float maxX = std::max(0.0f, sourceW - destW);
    float maxY = std::max(0.0f, sourceH - destH);

    float x = ofRandom(0, maxX);
    float y = ofRandom(0, maxY);

    captureCrop(dest, source, glm::vec2 { x, y });
}

void MemoryBank::captureCrop(ofFbo& dest, const ofFbo& source, glm::vec2 cropTopLeft) {
    float sourceW = source.getWidth();
    float sourceH = source.getHeight();
    float destW = dest.getWidth();
    float destH = dest.getHeight();

    float maxX = std::max(0.0f, sourceW - destW);
    float maxY = std::max(0.0f, sourceH - destH);

    float x = ofClamp(cropTopLeft.x, 0.0f, maxX);
    float y = ofClamp(cropTopLeft.y, 0.0f, maxY);

    dest.begin();
    ofClear(0, 0, 0, 0);
    ofPushStyle();
    ofEnableBlendMode(OF_BLENDMODE_DISABLED);
    ofSetColor(255);
    source.getTexture().drawSubsection(0, 0, destW, destH, x, y, destW, destH);
    ofPopStyle();
    dest.end();
}


const ofTexture* MemoryBank::select(float centre, float width) const {
    if (occupiedCount == 0 || saveOrder.empty()) return nullptr;

    int maxIndex = static_cast<int>(saveOrder.size()) - 1;
    int index = selectSlotIndex(centre, width, maxIndex);
    index = std::clamp(index, 0, maxIndex);

    int slot = saveOrder[index];
    return get(slot);
}

const ofTexture* MemoryBank::selectWeightedRecent(float centre, float width) const {
    if (occupiedCount == 0 || saveOrder.empty()) return nullptr;

    int maxIndex = static_cast<int>(saveOrder.size()) - 1;
    int index = selectSlotIndexWeighted(centre, width, true, maxIndex);
    index = std::clamp(index, 0, maxIndex);

    int slot = saveOrder[index];
    return get(slot);
}

const ofTexture* MemoryBank::selectWeightedOld(float centre, float width) const {
    if (occupiedCount == 0 || saveOrder.empty()) return nullptr;

    int maxIndex = static_cast<int>(saveOrder.size()) - 1;
    int index = selectSlotIndexWeighted(centre, width, false, maxIndex);
    index = std::clamp(index, 0, maxIndex);

    int slot = saveOrder[index];
    return get(slot);
}

const ofTexture* MemoryBank::selectRandom() const {
    if (occupiedCount == 0 || saveOrder.empty()) return nullptr;

    int index = static_cast<int>(ofRandom(0, static_cast<float>(saveOrder.size())));
    index = std::clamp(index, 0, static_cast<int>(saveOrder.size()) - 1);

    int slot = saveOrder[index];
    return get(slot);
}

const ofTexture* MemoryBank::get(int slot) const {
    if (!isValidSlotIndex(slot)) return nullptr;
    if (!occupied[slot]) return nullptr;
    if (!slots[slot].isAllocated()) return nullptr;
    return &slots[slot].getTexture();
}

bool MemoryBank::isOccupied(int slot) const {
    if (!isValidSlotIndex(slot)) return false;
    return occupied[slot] && slots[slot].isAllocated();
}

bool MemoryBank::saveAllToFolder(const std::filesystem::path& folder) const {
    if (!allocated) {
        ofLogError("MemoryBank") << "Cannot save: not allocated";
        return false;
    }

    std::error_code ec;
    std::filesystem::create_directories(folder, ec);
    if (ec) {
        ofLogError("MemoryBank") << "Failed to create memory folder: " << folder << " (" << ec.message() << ")";
        return false;
    }

    bool ok = true;

    for (int i = 0; i < NUM_SLOTS; ++i) {
        const std::filesystem::path path = getSlotFilePath(folder, i);

        if (occupied[i]) {
            ofPixels pixels;
            slots[i].readToPixels(pixels);
            ofPixels rgba = ensureRgbaPixels(pixels);

            if (!ofSaveImage(rgba, path.string())) {
                ofLogError("MemoryBank") << "Failed to save PNG: " << path;
                ok = false;
            }
        } else {
            std::error_code rmEc;
            if (std::filesystem::exists(path, rmEc)) {
                std::filesystem::remove(path, rmEc);
                if (rmEc) {
                    ofLogWarning("MemoryBank") << "Failed to remove stale memory PNG: " << path << " (" << rmEc.message() << ")";
                }
            }
        }
    }

    ofLogNotice("MemoryBank") << "Saved memories to folder: " << folder;
    return ok;
}

bool MemoryBank::loadAllFromFolder(const std::filesystem::path& folder) {
    if (!allocated) {
        ofLogError("MemoryBank") << "Cannot load: not allocated";
        return false;
    }

    clearAll();

    std::error_code ec;
    if (!std::filesystem::exists(folder, ec) || ec) {
        ofLogNotice("MemoryBank") << "No memory folder found: " << folder;
        return false;
    }

    bool anyLoaded = false;

    for (int i = 0; i < NUM_SLOTS; ++i) {
        const std::filesystem::path path = getSlotFilePath(folder, i);
        if (!std::filesystem::exists(path, ec) || ec) {
            ec.clear();
            continue;
        }

        ofImage img;
        if (!img.load(path.string())) {
            ofLogWarning("MemoryBank") << "Failed to load memory PNG: " << path;
            continue;
        }

        if (img.getWidth() != memorySize.x || img.getHeight() != memorySize.y) {
            img.resize(static_cast<int>(memorySize.x), static_cast<int>(memorySize.y));
        }

        slots[i].begin();
        ofClear(0, 0, 0, 0);
        ofPushStyle();
        ofEnableBlendMode(OF_BLENDMODE_DISABLED);
        ofSetColor(255);
        img.draw(0, 0, slots[i].getWidth(), slots[i].getHeight());
        ofPopStyle();
        slots[i].end();

        occupied[i] = true;
        occupiedCount++;
        saveOrder.push_back(i);
        anyLoaded = true;
    }

    if (anyLoaded) {
        ofLogNotice("MemoryBank") << "Loaded " << occupiedCount << " memories from folder: " << folder;
    }

    return anyLoaded;
}

void MemoryBank::clear(int slot) {
    if (!isValidSlotIndex(slot)) return;

    if (slots[slot].isAllocated()) {
        slots[slot].begin();
        ofClear(0, 0, 0, 0);
        slots[slot].end();
    }

    if (occupied[slot]) {
        occupied[slot] = false;
        occupiedCount = std::max(0, occupiedCount - 1);
    }

    removeSlotFromOrder(saveOrder, slot);

    if (pendingSaveSlot == slot) {
        pendingSaveSlot = -1;
    }
}

void MemoryBank::clearAll() {
    for (int i = 0; i < NUM_SLOTS; i++) {
        if (slots[i].isAllocated()) {
            slots[i].begin();
            ofClear(0, 0, 0, 0);
            slots[i].end();
        }
    }

    occupied.fill(false);
    occupiedCount = 0;
    saveOrder.clear();
    pendingSaveSlot = -1;
}

int MemoryBank::selectSlotIndex(float centre, float width, int maxIndex) const {
    if (maxIndex <= 0) return 0;

    float target = centre * static_cast<float>(maxIndex);
    float halfSpread = width * static_cast<float>(maxIndex) * 0.5f;
    float selected = target + ofRandom(-halfSpread, halfSpread);

    int index = static_cast<int>(std::round(selected));
    return std::clamp(index, 0, maxIndex);
}

int MemoryBank::selectSlotIndexWeighted(float centre, float width, bool preferRecent, int maxIndex) const {
    if (maxIndex <= 0) return 0;

    float target = centre * static_cast<float>(maxIndex);
    float halfSpread = width * static_cast<float>(maxIndex) * 0.5f;
    float baseSelected = target + ofRandom(-halfSpread, halfSpread);

    float bias = ofRandom(0.0f, 1.0f);
    bias = bias * bias;

    float weighted;
    if (preferRecent) {
        weighted = ofLerp(baseSelected, static_cast<float>(maxIndex), bias * 0.5f);
    } else {
        weighted = ofLerp(baseSelected, 0.0f, bias * 0.5f);
    }

    int index = static_cast<int>(std::round(weighted));
    return std::clamp(index, 0, maxIndex);
}



} // namespace ofxMarkSynth
