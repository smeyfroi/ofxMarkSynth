#pragma once

#include "ofMain.h"

#include <algorithm>
#include <string>
#include <vector>

namespace ofxMarkSynth {

// Holds a small discrete palette for a "key colour" register.
// Flip behavior intentionally matches Mod::changeDrawingLayer():
// - if current index is 0, switch to a random non-zero index
// - else switch back to 0
class ColorRegister {
public:
  void setColours(std::vector<ofFloatColor> colours_) {
    colours = std::move(colours_);
    if (colours.empty()) {
      currentIndex = 0;
    } else {
      currentIndex = std::clamp(currentIndex, 0, static_cast<int>(colours.size() - 1));
    }
  }

  const std::vector<ofFloatColor>& getColours() const { return colours; }

  int getCurrentIndex() const { return currentIndex; }

  bool isUsable() const { return colours.size() >= 2; }

  const ofFloatColor& getCurrentColour() const {
    static const ofFloatColor fallback { 0.0f, 0.0f, 0.0f, 1.0f };
    if (colours.empty()) return fallback;
    int idx = std::clamp(currentIndex, 0, static_cast<int>(colours.size() - 1));
    return colours[idx];
  }

  void flip() {
    if (colours.size() <= 1) {
      currentIndex = 0;
      return;
    }

    if (currentIndex == 0) {
      int newIndex = 1 + static_cast<int>(ofRandom(0, static_cast<float>(colours.size() - 1)));
      newIndex = std::clamp(newIndex, 1, static_cast<int>(colours.size() - 1));
      currentIndex = newIndex;
    } else {
      currentIndex = 0;
    }
  }

  // Parse a pipe-separated list of rgba values, e.g.
  // "0,0,0,0.3 | 0.5,0.5,0.5,0.3 | 1,1,1,0.3"
  static std::vector<ofFloatColor> parsePipeSeparatedColours(const std::string& s) {
    std::vector<ofFloatColor> out;

    auto parts = ofSplitString(s, "|", true, true);
    for (auto& part : parts) {
      std::string trimmed = ofTrim(part);
      if (trimmed.empty()) continue;

      auto comps = ofSplitString(trimmed, ",", true, true);
      if (comps.size() != 4) {
        ofLogWarning("ColorRegister") << "Bad colour entry (expected 4 comps): '" << trimmed << "'";
        continue;
      }

      float r = ofToFloat(ofTrim(comps[0]));
      float g = ofToFloat(ofTrim(comps[1]));
      float b = ofToFloat(ofTrim(comps[2]));
      float a = ofToFloat(ofTrim(comps[3]));
      out.emplace_back(r, g, b, a);
    }

    return out;
  }

  // Initialize once from a pipe-separated config string, with a fallback when empty.
  void ensureInitialized(bool& initialized, const std::string& serializedColours, const ofFloatColor& fallback0) {
    if (initialized) return;
    auto parsed = parsePipeSeparatedColours(serializedColours);
    if (parsed.empty()) {
      parsed.push_back(fallback0);
    }
    setColours(std::move(parsed));
    initialized = true;
  }

private:
  std::vector<ofFloatColor> colours;
  int currentIndex { 0 };
};

} // namespace ofxMarkSynth
