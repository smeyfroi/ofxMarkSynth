//
//  FontCache.hpp
//  ofxMarkSynth
//
//  Pre-loaded font cache with size binning for efficient text rendering.
//

#pragma once

#include "ofTrueTypeFont.h"
#include <array>
#include <filesystem>
#include <memory>
#include <unordered_map>

namespace ofxMarkSynth {

class FontCache {
public:
  // Pre-defined font size bins (exponential spacing)
  // Covers normalized font sizes 0.01-0.08 at 7200px FBO height (72px-576px)
  static constexpr std::array<int, 10> SIZE_BINS = {
    72, 96, 128, 160, 200, 256, 320, 400, 500, 560
  };

  explicit FontCache(const std::filesystem::path& fontPath);

  // Load all font sizes synchronously. Call before first use.
  void preloadAll();

  // Get font for requested pixel size. Returns nearest cached bin.
  // Returns nullptr if fonts not loaded or load failed.
  std::shared_ptr<ofTrueTypeFont> get(int requestedPixelSize);

  // Snap a pixel size to the nearest bin
  static int snapToBin(int pixelSize);

  // Check if all fonts are loaded
  bool isLoaded() const { return loaded; }

private:
  std::filesystem::path fontPath;
  std::unordered_map<int, std::shared_ptr<ofTrueTypeFont>> fonts;
  bool loaded { false };
};

} // namespace ofxMarkSynth
