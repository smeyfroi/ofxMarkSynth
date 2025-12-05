//
//  MemoryBank.hpp
//  ofxMarkSynth
//
//  Memory bank for storing texture fragments captured from the composite
//  during live performance, enabling recall of earlier visual states.
//

#pragma once

#include "ofFbo.h"
#include "ofTexture.h"
#include <array>



namespace ofxMarkSynth {



class MemoryBank {
public:
    static constexpr int NUM_SLOTS = 8;
    
    MemoryBank() = default;
    
    /// Allocate FBOs for all slots at the specified size
    void allocate(glm::vec2 memorySize, GLint internalFormat = GL_RGBA8);
    
    /// Save a random crop from source into a slot selected by centre/width
    /// @param source The source FBO to crop from (typically the full composite)
    /// @param centre Relative position in filled slots range [0,1], 1.0 = most recent
    /// @param width Selection spread [0,1], 0 = exact, 1 = full range
    /// @return The slot index used
    int save(const ofFbo& source, float centre, float width);
    
    /// Save a random crop to a specific slot (for manual GUI saves)
    void saveToSlot(const ofFbo& source, int slot);
    
    /// Select and return a memory texture using centre/width
    /// @return Pointer to texture, or nullptr if no memories exist
    const ofTexture* select(float centre, float width) const;
    
    /// Select with weighting toward recent memories
    const ofTexture* selectWeightedRecent(float centre, float width) const;
    
    /// Select with weighting toward old memories
    const ofTexture* selectWeightedOld(float centre, float width) const;
    
    /// Select purely random from filled slots
    const ofTexture* selectRandom() const;
    
    /// Direct slot access (returns nullptr if slot is empty or out of range)
    const ofTexture* get(int slot) const;
    
    /// Check if a slot contains a memory
    bool isOccupied(int slot) const;
    
    /// Number of filled slots (0 to NUM_SLOTS)
    int getFilledCount() const { return filledCount; }
    
    /// Get the configured memory size
    glm::vec2 getMemorySize() const { return memorySize; }
    
    /// Clear a specific slot (does not reallocate, just marks as empty)
    void clear(int slot);
    
    /// Clear all slots
    void clearAll();
    
private:
    std::array<ofFbo, NUM_SLOTS> slots;
    int filledCount { 0 };
    glm::vec2 memorySize { 1024, 1024 };
    bool allocated { false };
    
    /// Capture a random crop from source into the destination FBO
    void captureRandomCrop(ofFbo& dest, const ofFbo& source);
    
    /// Map centre/width to a slot index
    /// @param maxSlot Maximum slot index (inclusive) - for save this is filledCount, for emit it's filledCount-1
    int selectSlotIndex(float centre, float width, int maxSlot) const;
    
    /// Map centre/width with weighting toward one end
    /// @param preferRecent If true, weight toward higher indices (recent); if false, toward lower (old)
    int selectSlotIndexWeighted(float centre, float width, bool preferRecent) const;
};



} // namespace ofxMarkSynth
