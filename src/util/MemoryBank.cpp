//
//  MemoryBank.cpp
//  ofxMarkSynth
//
//  Memory bank for storing texture fragments captured from the composite
//  during live performance, enabling recall of earlier visual states.
//

#include "MemoryBank.hpp"
#include "ofGraphics.h"
#include "ofLog.h"
#include "ofMath.h"
#include <cmath>



namespace ofxMarkSynth {



void MemoryBank::allocate(glm::vec2 size, GLint internalFormat) {
    memorySize = size;
    for (int i = 0; i < NUM_SLOTS; i++) {
        slots[i].allocate(memorySize.x, memorySize.y, internalFormat);
        slots[i].begin();
        ofClear(0, 0, 0, 0);
        slots[i].end();
    }
    allocated = true;
    filledCount = 0;
}

int MemoryBank::save(const ofFbo& source, float centre, float width) {
    if (!allocated) {
        ofLogError("MemoryBank") << "Cannot save: not allocated";
        return -1;
    }
    
    // Select slot: for saving, maxSlot = filledCount (allows creating new slot)
    int maxSlot = std::min(filledCount, NUM_SLOTS - 1);
    int slot = selectSlotIndex(centre, width, maxSlot);
    
    // If we selected a slot beyond current fill, extend filledCount
    if (slot >= filledCount && filledCount < NUM_SLOTS) {
        slot = filledCount;
        filledCount++;
    }
    
    saveToSlot(source, slot);
    return slot;
}

void MemoryBank::saveToSlot(const ofFbo& source, int slot) {
    if (!allocated) {
        ofLogError("MemoryBank") << "Cannot save: not allocated";
        return;
    }
    if (slot < 0 || slot >= NUM_SLOTS) {
        ofLogError("MemoryBank") << "Invalid slot index: " << slot;
        return;
    }
    
    captureRandomCrop(slots[slot], source);
    
    // Update filledCount if we're filling a new slot
    if (slot >= filledCount) {
        filledCount = slot + 1;
    }
}

void MemoryBank::captureRandomCrop(ofFbo& dest, const ofFbo& source) {
    // Calculate random position for crop
    float sourceW = source.getWidth();
    float sourceH = source.getHeight();
    float destW = dest.getWidth();
    float destH = dest.getHeight();
    
    // Ensure we don't go out of bounds
    float maxX = std::max(0.0f, sourceW - destW);
    float maxY = std::max(0.0f, sourceH - destH);
    
    float x = ofRandom(0, maxX);
    float y = ofRandom(0, maxY);
    
    // Copy the subsection using the efficient drawSubsection method
    dest.begin();
    ofClear(0, 0, 0, 0);
    ofEnableBlendMode(OF_BLENDMODE_DISABLED);
    ofSetColor(255);
    source.getTexture().drawSubsection(0, 0, destW, destH, x, y);
    dest.end();
}

const ofTexture* MemoryBank::select(float centre, float width) const {
    if (filledCount == 0) return nullptr;
    
    int slot = selectSlotIndex(centre, width, filledCount - 1);
    return &slots[slot].getTexture();
}

const ofTexture* MemoryBank::selectWeightedRecent(float centre, float width) const {
    if (filledCount == 0) return nullptr;
    
    int slot = selectSlotIndexWeighted(centre, width, true);
    return &slots[slot].getTexture();
}

const ofTexture* MemoryBank::selectWeightedOld(float centre, float width) const {
    if (filledCount == 0) return nullptr;
    
    int slot = selectSlotIndexWeighted(centre, width, false);
    return &slots[slot].getTexture();
}

const ofTexture* MemoryBank::selectRandom() const {
    if (filledCount == 0) return nullptr;
    
    int slot = static_cast<int>(ofRandom(0, filledCount));
    slot = std::clamp(slot, 0, filledCount - 1);
    return &slots[slot].getTexture();
}

const ofTexture* MemoryBank::get(int slot) const {
    if (slot < 0 || slot >= filledCount) return nullptr;
    if (!slots[slot].isAllocated()) return nullptr;
    return &slots[slot].getTexture();
}

bool MemoryBank::isOccupied(int slot) const {
    return slot >= 0 && slot < filledCount && slots[slot].isAllocated();
}

void MemoryBank::clear(int slot) {
    if (slot < 0 || slot >= NUM_SLOTS) return;
    
    // Clear the FBO content
    if (slots[slot].isAllocated()) {
        slots[slot].begin();
        ofClear(0, 0, 0, 0);
        slots[slot].end();
    }
    
    // If clearing the last filled slot, reduce count
    // Otherwise we leave a gap (which is fine for now)
    if (slot == filledCount - 1) {
        filledCount--;
        // Continue reducing if there are more empty slots at the end
        while (filledCount > 0) {
            // For simplicity, we just reduce the count
            // In a more sophisticated version, we'd track individual slot occupancy
            break;
        }
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
    filledCount = 0;
}

int MemoryBank::selectSlotIndex(float centre, float width, int maxSlot) const {
    if (maxSlot < 0) return 0;
    if (maxSlot == 0) return 0;
    
    // Centre maps [0,1] to [0, maxSlot]
    float targetSlot = centre * maxSlot;
    
    // Width determines spread: 0 = exact, 1 = full range
    float halfSpread = width * maxSlot * 0.5f;
    
    // Random within range
    float selected = targetSlot + ofRandom(-halfSpread, halfSpread);
    
    // Clamp to valid range
    int slot = static_cast<int>(std::round(selected));
    return std::clamp(slot, 0, maxSlot);
}

int MemoryBank::selectSlotIndexWeighted(float centre, float width, bool preferRecent) const {
    if (filledCount == 0) return 0;
    if (filledCount == 1) return 0;
    
    int maxSlot = filledCount - 1;
    
    // Start with centre/width selection
    float targetSlot = centre * maxSlot;
    float halfSpread = width * maxSlot * 0.5f;
    float baseSelected = targetSlot + ofRandom(-halfSpread, halfSpread);
    
    // Apply weighting bias
    // For preferRecent: bias toward higher indices
    // For preferOld: bias toward lower indices
    float bias = ofRandom(0.0f, 1.0f);
    bias = bias * bias; // Square for non-linear weighting
    
    float weighted;
    if (preferRecent) {
        // Bias toward maxSlot
        weighted = ofLerp(baseSelected, maxSlot, bias * 0.5f);
    } else {
        // Bias toward 0
        weighted = ofLerp(baseSelected, 0.0f, bias * 0.5f);
    }
    
    int slot = static_cast<int>(std::round(weighted));
    return std::clamp(slot, 0, maxSlot);
}



} // namespace ofxMarkSynth
