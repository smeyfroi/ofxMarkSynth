# Synth config setup

This addon loads “synth configs” from JSON files and (optionally) shows them in the Performance grid UI.

## Folder layout

Performance configs are loaded by `ofxMarkSynth::PerformanceNavigator` from a folder under the app-provided resource `performanceConfigRootPath`.

- `performanceConfigRootPath/`
  - `synth/` (required)
    - `MyConfigA.json`
    - `MyConfigA.jpg` (optional thumbnail)
    - `MyConfigB.json`
    - `MyConfigB.jpeg` (optional thumbnail)

Only `*.json` files in the `synth/` folder are treated as configs.

## JSON structure

Full config contents are described in `docs/synth-config-reference.json`.

In addition to the main synth config schema (mods, layers, etc), there are a few top-level metadata keys used by the Performance UI.

### `description`

Optional string shown in the Performance grid tooltip.

### `buttonGrid`

Optional object used to control Performance grid placement and color.

Example:

```json
{
  "description": "Warm, slow-moving particles.",
  "buttonGrid": {
    "x": 0,
    "y": 0,
    "color": "#FF8800"
  },
  "version": "1.0",
  "synth": { /* ... */ },
  "drawingLayers": { /* ... */ },
  "mods": { /* ... */ }
}
```

- `x`: integer grid column (0–7)
- `y`: integer grid row (0–6)
- `color`: hex string `#RRGGBB` (used for the grid pad color)

If `buttonGrid` is missing, configs are auto-assigned to free cells.

### Underscore-prefixed keys

Keys starting with `_` are reserved for comments/documentation and are ignored by parameter application.

## Optional thumbnails

The Performance grid tooltip can show an image thumbnail.

- Filename convention (no JSON key required):
  - `MyConfig.json` → `MyConfig.jpg` or `MyConfig.jpeg` (same folder)
- `.png` is ignored.
- Size limit: thumbnails must be `<= 256px` in both width and height.
  - Larger images are rejected and an error is logged.

Thumbnails are loaded and uploaded to the GPU when the performance config folder is loaded (startup), to avoid hitches during live performance.
