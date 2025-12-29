//
//  FontStash2Cache.cpp
//  ofxMarkSynth
//

#include "core/FontStash2Cache.hpp"

namespace ofxMarkSynth {

FontStash2Cache::FontStash2Cache(const std::filesystem::path& fontPath_)
    : fontPath(fontPath_)
    , fontId("mainFont")
{
}

FontStash2Cache::FontStash2Cache(FontStash2Cache&& other) noexcept
    : fonts(std::move(other.fonts))
    , fontPath(std::move(other.fontPath))
    , fontId(std::move(other.fontId))
    , setupComplete(other.setupComplete)
    , ready(other.ready)
{
    other.setupComplete = false;
    other.ready = false;
}

FontStash2Cache& FontStash2Cache::operator=(FontStash2Cache&& other) noexcept {
    if (this != &other) {
        fonts = std::move(other.fonts);
        fontPath = std::move(other.fontPath);
        fontId = std::move(other.fontId);
        setupComplete = other.setupComplete;
        ready = other.ready;
        other.setupComplete = false;
        other.ready = false;
    }
    return *this;
}

void FontStash2Cache::setup() {
    if (setupComplete) return;

    // Initialize nanoVG context
    bool debug = false;
    fonts.setup(debug);

    // Load the font
    if (!fonts.addFont(fontId, fontPath.string())) {
        ofLogError("FontStash2Cache") << "Failed to load font: " << fontPath;
        return;
    }

    ofLogNotice("FontStash2Cache") << "Font loaded: " << fontPath;
    setupComplete = true;
}

void FontStash2Cache::prewarmAll() {
    if (!setupComplete) {
        ofLogWarning("FontStash2Cache") << "prewarmAll called before setup";
        return;
    }

    ofLogNotice("FontStash2Cache") << "Prewarming " << FONT_SIZE_BINS.size() << " size bins...";

    for (int size : FONT_SIZE_BINS) {
        prewarmSize(size);
    }

    ready = true;
    ofLogNotice("FontStash2Cache") << "Prewarm complete";
}

void FontStash2Cache::prewarmSize(int fontSize) {
    // Build string with all ASCII printable characters
    std::string chars;
    chars.reserve(PREWARM_CHAR_END - PREWARM_CHAR_START + 1);
    for (uint32_t c = PREWARM_CHAR_START; c <= PREWARM_CHAR_END; ++c) {
        chars += static_cast<char>(c);
    }

    // Create style for this size
    ofxFontStash2::Style style(fontId, static_cast<float>(fontSize), ofColor::white);

    // getTextBounds triggers glyph rasterization without actually drawing
    fonts.getTextBounds(chars, style, 0, 0);

    ofLogVerbose("FontStash2Cache") << "Prewarmed size " << fontSize;
}

int FontStash2Cache::snapToBin(int requestedPixelSize) {
    // Find the closest bin
    int closest = FONT_SIZE_BINS[0];
    int minDiff = std::abs(requestedPixelSize - closest);

    for (size_t i = 1; i < FONT_SIZE_BINS.size(); ++i) {
        int diff = std::abs(requestedPixelSize - FONT_SIZE_BINS[i]);
        if (diff < minDiff) {
            minDiff = diff;
            closest = FONT_SIZE_BINS[i];
        }
    }

    return closest;
}

ofxFontStash2::Style FontStash2Cache::createStyle(int requestedPixelSize, const ofFloatColor& color) const {
    int snappedSize = snapToBin(requestedPixelSize);
    
    // Convert ofFloatColor to ofColor (0-255 range)
    ofColor c(
        static_cast<unsigned char>(color.r * 255),
        static_cast<unsigned char>(color.g * 255),
        static_cast<unsigned char>(color.b * 255),
        static_cast<unsigned char>(color.a * 255)
    );

    return ofxFontStash2::Style(fontId, static_cast<float>(snappedSize), c);
}

ofRectangle FontStash2Cache::getTextBounds(const std::string& text, const ofxFontStash2::Style& style, float x, float y) {
    return fonts.getTextBounds(text, style, x, y);
}

float FontStash2Cache::draw(const std::string& text, const ofxFontStash2::Style& style, float x, float y) {
    return fonts.draw(text, style, x, y);
}

} // namespace ofxMarkSynth
