# Performance References

This document catalogues tested MarkSynth performance configuration sets. Use these as reference points for parameter ranges, architectural patterns, and proven aesthetic combinations.

---

## CaldewRiver

**Location**: `/Users/steve/Documents/MarkSynth-performances/CaldewRiver`

**Description**: First complete MarkSynth performance, premiered November 2025. A 45-minute structured piece responding to the River Caldew in Cumbria.

**Structure**: 5 movements with 41 synth configs, progressing from sparse/still to dense/turbulent and back.

**Key Reference Material**:

- **ParticleSet configurations**: 6+ configs with fully specified parameters for connection-line aesthetics
  - `21-movement3-cataract-a.json` through `25-movement3-cataract-a-exhausted.json`
  - `31-movement4-meander-base.json` (dual ParticleSet: camera + audio driven)
  - `41-movement5-delta-cycle1.json`

- **Tested ParticleSet parameter ranges** (from CaldewRiver configs):
  - `maxParticles`: 749–1500
  - `maxParticleAge`: 91–500 (shorter = more dynamic)
  - `connectionRadius`: 0.089–0.159 (key for web density)
  - `attractionStrength`: -0.1 to +0.03 (negative = repel/spread, positive = cluster)
  - `colourMultiplier`: 0.5–1.0
  - `lineFadeExponent`: 1.0–2.0

- **Dual ParticleSet pattern** (`31-movement4-meander-base.json`):
  - `CameraParticleSet`: positive `attractionStrength` (0.03), clustering behavior
  - `AudioParticleSet`: negative `attractionStrength` (-0.05), spreading behavior
  - Different layers, different character

**Notes**: CaldewRiver configs predate the Intent system refinements and use older layer naming conventions. Parameter values are reliable; structural patterns may need adaptation.

---

## Improvisation1

**Location**: `/Users/steve/Documents/MarkSynth-performances/Improvisation1`

**Description**: 8×7 improvisation grid for live performance. Each row is an "instrument family" with consistent modality columns.

**Structure**:
- 56 configs total (8 columns × 7 rows)
- Columns 0–3: A/V (audio + video)
- Columns 4–5: Audio-driven
- Columns 6–7: Video-driven

**Row Themes**:
| Row | Prefix | Theme | Status |
|-----|--------|-------|--------|
| 0 | 00–07 | `minimal` | Complete |
| 1 | 10–17 | `palette-smear` | Complete |
| 2 | 20–27 | `fluid-carriers` | Complete |
| 3 | 30–37 | `dualfield-particles` | Complete |
| 4 | 40–47 | `geometry-architecture` | Complete |
| 5 | 50–57 | `memory-collage` | Planned |
| 6 | 60–67 | `chaos-bank` | Planned |

**Key Reference Material**:

- **Row 1 (palette-smear)**: Smear as main persistence; SomPalette-derived colors
  - `12-palette-smear-av-particleseed.json`: ParticleSet seeding surface

- **Row 3 (dualfield-particles)**: Two ParticleFields + DividedArea
  - Demonstrates ParticleInk (into fluid) + foreground ParticleField pattern
  - Split DividedArea routing (minor → fluid, major → overlay)

- **Row 4 (geometry-architecture)**: DividedArea primary + ParticleSet secondary
  - First row to foreground ParticleSet connection-line webs
  - Architectural/geometric aesthetic with organic contrast

**Design Notes**: See `improvisation-grid-design-notes.md` for detailed layer budgets, parameter guidelines, and row-specific patterns.

---

## Adding New References

When completing a new performance or config set, add an entry here with:

1. **Location**: Filesystem path
2. **Description**: Brief artistic/technical summary
3. **Structure**: How configs are organized
4. **Key Reference Material**: Specific configs worth studying, notable patterns
5. **Notes**: Caveats, version considerations, adaptation guidance
