//
//  FontCache.cpp
//  ofxMarkSynth
//

#include "FontCache.hpp"
#include "ofLog.h"
#include <algorithm>

namespace ofxMarkSynth {

FontCache::FontCache(const std::filesystem::path& fontPath_)
  : fontPath { fontPath_ }
{
}

void FontCache::preloadAll() {
  if (loaded) return;

  for (int size : SIZE_BINS) {
    auto fontPtr = std::make_shared<ofTrueTypeFont>();
    bool success = fontPtr->load(fontPath.string(), size, true, true);
    if (success) {
      fonts[size] = fontPtr;
    } else {
      ofLogError("FontCache") << "Failed to load font " << fontPath << " at size " << size;
    }
  }

  loaded = true;
  ofLogNotice("FontCache") << "Pre-loaded " << fonts.size() << " font sizes from " << fontPath;
}

int FontCache::snapToBin(int pixelSize) {
  auto it = std::lower_bound(SIZE_BINS.begin(), SIZE_BINS.end(), pixelSize);
  if (it == SIZE_BINS.end()) return SIZE_BINS.back();
  if (it == SIZE_BINS.begin()) return SIZE_BINS.front();
  return (pixelSize - *(it - 1) < *it - pixelSize) ? *(it - 1) : *it;
}

std::shared_ptr<ofTrueTypeFont> FontCache::get(int requestedPixelSize) {
  if (!loaded) {
    ofLogWarning("FontCache") << "Fonts not loaded, call preloadAll() first";
    return nullptr;
  }

  int binSize = snapToBin(requestedPixelSize);
  auto it = fonts.find(binSize);
  if (it != fonts.end()) {
    return it->second;
  }

  // Fallback: find any available font
  if (!fonts.empty()) {
    return fonts.begin()->second;
  }

  return nullptr;
}

} // namespace ofxMarkSynth
