# Vendored ofxGist / Gist changes (MarkSynth)

MarkSynth uses `ofxGist` (and its vendored `Gist` library) for local audio analysis (`LocalGistClient`).

We intentionally patch some vendored code to make the analysis usable for our musical baseline
(trombone low fundamentals through piccolo-ish highs) and to avoid sample-rate mismatches.

## Changes made

- YIN pitch detector max frequency increased
  - File: `addons/ofxGist/libs/Gist/src/pitch/Yin.cpp`
  - Change: `setMaxFrequency(1500)` -> `setMaxFrequency(4500)`
  - Reason: 1500 Hz is too low for upper-register instruments.

- YIN now respects its configured `minPeriod` (via `setMaxFrequency`)
  - File: `addons/ofxGist/libs/Gist/src/pitch/Yin.cpp`
  - Change: removed hardcoded `minPeriod = 30` in `getPeriodCandidate()` and use the class `minPeriod`.
  - Reason: hardcoded `minPeriod` silently caps pitch and makes `setMaxFrequency()` ineffective.

- YIN constructor no longer calls `setSamplingFrequency()` with uninitialized state
  - File: `addons/ofxGist/libs/Gist/src/pitch/Yin.cpp`
  - Change: initialize `fs`/`minPeriod` directly, then call `setMaxFrequency()`.
  - Reason: the previous constructor path relied on undefined initial values (safe in practice because `setMaxFrequency()` overwrote
    `minPeriod`, but still risky/undefined).

- Added `Gist::setSamplingFrequency()` and wired it from `ofxGist`
  - Files:
    - `addons/ofxGist/libs/Gist/src/Gist.h`
    - `addons/ofxGist/src/ofxGist.cpp`
  - Reason: if the audio stream sample-rate changes (e.g. 44.1k vs 48k), pitch and MFCC should stay correct.

## Related (not ofxGist, but interacts strongly)

- `LocalGistClient` now overrides its `sampleRate` from the loaded WAV file’s sample-rate.
  - File: `addons/ofxAudioAnalysisClient/src/LocalGistClient.cpp`
  - Reason: prevents pitch computation from using a mismatched ctor/sample-rate.

## Notes

- Pitch accuracy (especially low fundamentals) depends strongly on analysis frame size (`bufferSize`).
  For baseline tuning we use larger buffers (e.g. 2048 @ 44.1kHz).
