//
//  CueGlyphController.hpp
//  ofxMarkSynth
//
//  Draws subtle performer cue glyphs in window space.
//

#pragma once

namespace ofxMarkSynth {

class CueGlyphController {
public:
  struct DrawParams {
    bool audioEnabled { false };
    bool videoEnabled { false };
    float alpha { 0.15f }; // 0..1 overall opacity

    // 0..1, only meaningful near end of config duration.
    float imminentConfigChangeProgress { 0.0f };

    // When true, draw an expired-time warning (typically flashed externally).
    bool flashExpired { false };
  };

  CueGlyphController() = default;

  void draw(const DrawParams& params, float windowWidth, float windowHeight) const;
};

} // namespace ofxMarkSynth
