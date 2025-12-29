//
//  FontStash2Cache.hpp
//  ofxMarkSynth
//
//  Wrapper around ofxFontStash2 with size binning and prewarming support.
//

#pragma once

#include "ofMain.h"
#include "ofxFontStash2.h"
#include <filesystem>
#include <array>

namespace ofxMarkSynth {

// Size bins for font rendering - requests are snapped to nearest bin
// to avoid excessive glyph rasterization at many unique sizes
constexpr std::array<int, 10> FONT_SIZE_BINS = {72, 96, 128, 160, 200, 256, 320, 400, 500, 560};

// Character range for prewarming (ASCII printable)
constexpr uint32_t PREWARM_CHAR_START = 32;
constexpr uint32_t PREWARM_CHAR_END = 126;

/**
 * FontStash2Cache - Font cache wrapper using ofxFontStash2
 *
 * Features:
 * - Size binning to reduce unique glyph rasterization
 * - Synchronous prewarming of ASCII glyphs at all size bins
 * - Simple API for TextMod integration
 *
 * Usage:
 *   auto cache = std::make_shared<FontStash2Cache>(fontPath);
 *   cache->setup();      // Call after GL context ready
 *   cache->prewarmAll(); // Prewarm all size bins (blocks)
 *
 *   // At draw time:
 *   auto style = cache->createStyle(pixelSize, color);
 *   cache->getFonts().draw(text, style, x, y);
 */
class FontStash2Cache {
public:
    explicit FontStash2Cache(const std::filesystem::path& fontPath);
    ~FontStash2Cache() = default;

    // Non-copyable (owns ofxFontStash2::Fonts)
    FontStash2Cache(const FontStash2Cache&) = delete;
    FontStash2Cache& operator=(const FontStash2Cache&) = delete;

    // Movable
    FontStash2Cache(FontStash2Cache&& other) noexcept;
    FontStash2Cache& operator=(FontStash2Cache&& other) noexcept;

    /**
     * Initialize the font system. Must be called after GL context is ready.
     */
    void setup();

    /**
     * Prewarm all size bins with ASCII characters.
     * This blocks until complete - call during app setup.
     */
    void prewarmAll();

    /**
     * Check if setup and prewarming are complete.
     */
    bool isReady() const { return ready; }

    /**
     * Snap a requested pixel size to the nearest size bin.
     */
    static int snapToBin(int requestedPixelSize);

    /**
     * Create a Style for the given pixel size and color.
     * The size is snapped to the nearest bin.
     */
    ofxFontStash2::Style createStyle(int requestedPixelSize, const ofFloatColor& color) const;

    /**
     * Get text bounds for the given text and style.
     */
    ofRectangle getTextBounds(const std::string& text, const ofxFontStash2::Style& style, float x, float y);

    /**
     * Draw text at the given position.
     * Returns text width (advance).
     */
    float draw(const std::string& text, const ofxFontStash2::Style& style, float x, float y);

    /**
     * Access the underlying Fonts instance for advanced usage.
     */
    ofxFontStash2::Fonts& getFonts() { return fonts; }
    const ofxFontStash2::Fonts& getFonts() const { return fonts; }

private:
    void prewarmSize(int fontSize);

    ofxFontStash2::Fonts fonts;
    std::filesystem::path fontPath;
    std::string fontId;  // ID used to reference font in ofxFontStash2
    bool setupComplete = false;
    bool ready = false;
};

} // namespace ofxMarkSynth
