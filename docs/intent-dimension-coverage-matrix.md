# Intent Dimension Coverage Matrix (Mod Type × E/D/S/C/G)

This document is a **gap-finder**: it shows which Intent dimensions (Energy/Density/Structure/Chaos/Granularity) are actually used by each Mod type’s `applyIntent()` mapping.

Why this is useful:
- Helps spot **under-utilized dimensions** (e.g. `S` rarely mapped in the Mods you’re using).
- Helps spot **low-coverage Mods** that only respond to 1–2 dimensions (so multiple intents will feel similar).

## How to build the table (repeatable)

Inputs:
- A config set (e.g. `config/synth/*.json`) to determine which Mod `type`s are actually in use.
- `docs/intent-mappings.md` as the source-of-truth for which dimensions each Mod maps.

Method:
1. Enumerate unique Mod `type`s used by the config set.
2. For each Mod type, find its section in `docs/intent-mappings.md` (typically `XxxMod` or `XxxSourceMod`).
3. Collect dimensions used anywhere in that Mod’s mapping tables.
   - Expressions like `1-S` still count as using `S`.
4. Render a Markdown matrix (Mod type × {E,D,S,C,G}).

Notes:
- Some Mod types are intentionally *not* Intent-driven (e.g. `MultiplyAdd`, `SomPalette`, `AudioDataSource`). These should appear as “no intent mapping”.

## Example matrix — Improvisation1 config set

This matrix was built from the Improvisation1 `config/synth/*.json` set, cross-referenced against `docs/intent-mappings.md`.

Legend:
- `●` = dimension is used in that Mod’s `applyIntent()` mappings

| Mod type | present in configs | mapping ref | E | D | S | C | G |
|---|---:|---|:--:|:--:|:--:|:--:|:--:|
| AudioDataSource | 56 | (no intent mapping) |  |  |  |  |  |
| Cluster | 70 | ClusterMod |  |  |  | ● |  |
| Collage | 26 | CollageMod | ● | ● | ● | ● | ● |
| DividedArea | 32 | DividedAreaMod | ● | ● | ● | ● | ● |
| Fade | 123 | FadeMod |  | ● |  |  | ● |
| Fluid | 44 | FluidMod | ● | ● | ● | ● | ● |
| FluidRadialImpulse | 57 | FluidRadialImpulseMod | ● |  | ● | ● | ● |
| MultiplyAdd | 191 | (no intent mapping) |  |  |  |  |  |
| ParticleField | 23 | ParticleFieldMod | ● | ● | ● | ● | ● |
| ParticleSet | 21 | ParticleSetMod | ● | ● | ● | ● | ● |
| Path | 26 | PathMod |  | ● |  |  | ● |
| PixelSnapshot | 2 | PixelSnapshotMod |  |  | ● |  | ● |
| SandLine | 9 | SandLineMod | ● |  | ● | ● | ● |
| Smear | 22 | SmearMod | ● | ● | ● | ● | ● |
| SoftCircle | 105 | SoftCircleMod | ● | ● |  |  |  |
| SomPalette | 56 | (no intent mapping) |  |  |  |  |  |
| Text | 1 | TextMod | ● | ● |  | ● | ● |
| TextSource | 1 | TextSourceMod |  |  |  | ● |  |
| TimerSource | 2 | TimerSourceMod | ● |  |  |  |  |
| VideoFlowSource | 44 | VideoFlowSourceMod |  | ● |  |  |  |

Quick read (Improvisation1):
- `S` (Structure) is still less common than `E/D/C/G`, but it’s now present in more of the major motion Mods (e.g. `Fluid`, `FluidRadialImpulse`).
- `E`, `D`, `C`, `G` remain more broadly distributed.
