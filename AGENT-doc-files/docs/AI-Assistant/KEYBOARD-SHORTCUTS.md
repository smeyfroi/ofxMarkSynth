# Keyboard Shortcuts Reference

This document serves as the comprehensive reference for all keyboard shortcuts in ofxMarkSynth and its related addons. Use this when updating the help text in `src/util/HelpContent.hpp`.

## Help Text Location

The user-facing help text is defined in:
- **File:** `src/util/HelpContent.hpp`
- **Constant:** `HELP_CONTENT`
- **Displayed via:** `?` key toggles help window (handled in `Synth.cpp:635`)

## Format Guidelines

When updating `HELP_CONTENT`:
- No main title (window title serves this purpose)
- No underlines under section headers
- Use 16-character left column for keys, then description
- Group logically by function

---

## Complete Shortcut Reference

### Synth-Level (src/Synth.cpp)

| Key | Action | Line |
|-----|--------|------|
| `Tab` | Toggle GUI visibility | 633 |
| `?` | Toggle help window | 635 |
| `Space` | Pause / Resume | 637-643 |
| `H` | Toggle hibernation (fade out/in) | 646-652 |
| `S` | Save HDR image | 655-658 |
| `R` | Toggle video recording | 660-663 |

**Note:** `Tab` and `?` are omitted from help text as they are self-evident.

### Performance Navigation (src/util/PerformanceNavigator.cpp)

| Key | Action | Line |
|-----|--------|------|
| `Left Arrow` (hold ~1s) | Load previous config | 29-31 |
| `Right Arrow` (hold ~1s) | Load next config | 25-27 |

Hold threshold defined by `HOLD_THRESHOLD_MS` constant.

### GUI / ImGui (src/Gui.cpp)

| Key | Action | Line |
|-----|--------|------|
| `Ctrl+Z` | Undo last snapshot load | 973 |
| `F1` - `F8` | Load snapshot from slot 1-8 | 985-997 |

### Mod-Specific

#### PathMod (src/processMods/PathMod.cpp)
| Key | Action | Line |
|-----|--------|------|
| `A` | Toggle visibility | 152-155 |

#### PixelSnapshotMod (src/processMods/PixelSnapshotMod.cpp)
| Key | Action | Line |
|-----|--------|------|
| `X` | Toggle visibility | 78-81 |

#### AudioDataSourceMod (src/sourceMods/AudioDataSourceMod.cpp)
| Key | Action | Line |
|-----|--------|------|
| `T` | Toggle audio tuning display | 235-238 |

#### SomPaletteMod (delegates to ofxSomPalette)
| Key | Action | Source |
|-----|--------|--------|
| `C` | Toggle SOM palette visibility | ofxSomPalette/src/ofxSomPalette.cpp:120-123 |
| `U` | Save palette snapshot to ~/Documents/som/ | ofxSomPalette/src/ofxSomPalette.cpp:104-118 |

#### IntrospectorMod (delegates to ofxIntrospector)
| Key | Action | Source |
|-----|--------|--------|
| `i` | Toggle introspector visibility | ofxIntrospector/src/ofxIntrospector.cpp:89-92 |

#### VideoFlowSourceMod (delegates to ofxMotionFromVideo)
| Key | Action | Source |
|-----|--------|--------|
| `V` | Toggle video visibility | ofxMotionFromVideo/src/ofxMotionFromVideo.cpp:161-163 |
| `M` | Toggle motion visibility | ofxMotionFromVideo/src/ofxMotionFromVideo.cpp:164-166 |

### Audio Addon Shortcuts

#### Plots (ofxAudioData/src/Plots.cpp)
| Key | Action | Line |
|-----|--------|------|
| `p` | Toggle scalar plots visibility | 137-138 |
| `<` | Change plot to next analysis scalar | 131-133 |
| `>` | Change plot to previous analysis scalar | 134-136 |

#### SpectrumPlots (ofxAudioData/src/SpectrumPlots.cpp)
| Key | Action | Line |
|-----|--------|------|
| `P` | Toggle spectrum/MFCC plots visibility | 11-13 |

#### LocalGistClient (ofxAudioAnalysisClient/src/LocalGistClient.cpp)
| Key | Action | Line |
|-----|--------|------|
| `` ` `` (backtick) | Toggle audio mute | 228-231 |
| `Up Arrow` | Skip forward 10s (60s with Shift) | 238-244 |
| `Down Arrow` | Skip backward 10s (60s with Shift) | 247-252 |

#### FileClient (ofxAudioAnalysisClient/src/FileClient.cpp)
| Key | Action | Line |
|-----|--------|------|
| `` ` `` (backtick) | Toggle audio on/off | 30-34 |

---

## Key Handler Chain

The keyboard event flows through:

1. **ofApp::keyPressed()** - Example app level
2. **Synth::keyPressed()** - Checks ImGui capture first
3. **PerformanceNavigator::keyPressed()** - Arrow key hold detection
4. **Synth direct handlers** - Tab, ?, Space, H, S, R
5. **Mod::keyPressed()** - Iterates all mods, returns on first handler

```cpp
// From Synth.cpp
bool Synth::keyPressed(int key) {
  if (ImGui::GetIO().WantCaptureKeyboard) return false;
  if (performanceNavigator.keyPressed(key)) return true;
  // ... synth-level keys ...
  return std::any_of(modPtrs, [&](auto& mod) { return mod->keyPressed(key); });
}
```

---

## Adding New Shortcuts

1. Implement in appropriate `keyPressed()` method
2. Return `true` if handled to prevent propagation
3. Update this document
4. Update `src/util/HelpContent.hpp` if user-facing
