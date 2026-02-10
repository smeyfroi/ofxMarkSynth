# Intent “Around-Manual” Mapping — Migration Plan

Goal: reduce Intent blowouts (Maximum/Grain) by mapping intent targets **around the tuned manual value** rather than across full parameter min/max, while keeping the existing `lin/exp` behavior available for gradual migration.

This plan is written to be implementation-ready and to support incremental roll-out across Mods.

---

## Problem statement (what we’re fixing)

Many `applyIntent()` implementations use `IntentMapper` fluent terminals:

- `lin`: `lerp(min, max, v)`
- `exp`: `lerp(min, max, pow(v, n))`

Where `min/max` are the controller’s manual parameter bounds.

This can be too aggressive for parameters that:
- directly control presence/opacity (e.g. `AlphaMultiplier`, `Opacity`)
- control injection rate / activity density (e.g. `PointSamplesPerUpdate`)
- amplify feedback systems (`Smear` multipliers, `Fluid`/impulse strengths)

Result: high-density intents (e.g. “Grain”, “Maximum”) can saturate layers and destabilize motion systems, overriding carefully tuned per-config manual values.

---

## Desired behavior: “around-manual” mapping

Instead of mapping intent directly into `[min..max]`, map it into a **band around the current manual value** `m`:

- `lo = lerp(m, min, downAmount)`
- `hi = lerp(m, max, upAmount)`
- `target = lerp(lo, hi, curve(v))`

Where:
- `v` is the mapping value (e.g. `E`, `D`, `C*E`, etc.) in `[0..1]`
- `curve(v)` is either linear (`v`) or exponential (`pow(v, exponent)`)
- `upAmount/downAmount` are normalized band sizes in `[0..1]`

Notes:
- Use `downAmount > upAmount` by default (it’s safer to “calm” than to “overdrive”).
- Preserve existing behavior: only Mods explicitly migrated should use the new mapping.

---

## Implementation steps (sequenced)

### Step 1 — ParamController: manual anchor accessor

Add a small accessor to `ParamController<T>` so `IntentMapper` can read the current tuned manual value:

- `T getManualValue() const { return manualValueParameter.get(); }`

This is internal plumbing only (no new config params).

### Step 2 — IntentMapper: new terminals

Add new fluent terminals to `Mapping` (in `src/core/IntentMapper.hpp`):

- `linAround(ctrl, strength, upAmount, downAmount)`
- `expAround(ctrl, strength, upAmount, downAmount, exponent)`

Plus convenience overloads using global defaults:

- `linAround(ctrl, strength, RiskClass class)`
- `expAround(ctrl, strength, RiskClass class, exponent)`

(Implementation detail: RiskClass can be an enum in `IntentMapper.hpp` to keep constants co-located.)

### Step 3 — Global defaults (keep them in IntentMapper)

Define defaults inside `IntentMapper.hpp` (per the “no sprawl” preference).

First-pass defaults (more permissive than conservative values; intent is managed live):

- `Alpha`: `up=0.30`, `down=0.80`
- `Size`: `up=0.45`, `down=0.80`
- `Gain`: `up=0.35`, `down=0.70`
- `Force`: `up=0.30`, `down=0.70`
- `Speed`: `up=0.35`, `down=0.70`
- `Feedback`: `up=0.30`, `down=0.70`
- `Persistence`: `up=0.40`, `down=0.70`

### Step 4 — Selective migration by Mod (priority order)

Migrate high-risk mappings first; keep old `lin/exp` where it is known-safe.

#### Priority A (reported troublemakers)

1) `VideoFlowSourceMod`
- Migrate `PointSamplesPerUpdate` from `exp` to `expAround`.
- Treat as safety-critical: use a smaller `upAmount` override than the default (e.g. `up=0.20`), but allow some modulation.

2) `FluidRadialImpulseMod`
- Keep its custom formulas, but apply around-manual banding to final intent targets.
- Strongly consider tighter `upAmount` on strength-like parameters than radius-like parameters.

3) `FluidMod`
- Key decision: `dt` mapping.

  Options:
  - **(A) Exclude `dt` from intent** (recommended default): remove `E -> dt` mapping.
    - Rationale: `dt` needs to remain coherent with impulse shader dt semantics and global stability.
  - **(B) Keep `dt` in intent, but tie Fluid + FluidRadialImpulse**:
    - Apply the same around-manual mapping to both `Fluid.dt` and `FluidRadialImpulse.dt`.
    - Add a docs note: those two parameters are intentionally coupled.

  Regardless of dt decision, migrate other risky parameters to around-manual:
  - vorticity target
  - dissipation targets

4) `SoftCircleMod`
- Migrate `D -> AlphaMultiplier` to `linAround(Alpha)`.
- Migrate `E -> Radius` to `expAround(Size)`.
- Replace hard-coded `D -> ColourMultiplier [1.0..1.4]` with around-manual gain mapping so it respects config tuning.

5) `ParticleSetMod`
- Migrate `timeStep`, `forceScale`, `connectionRadius`, `colourMultiplier`, `maxSpeed`.

6) `ParticleFieldMod`
- Migrate force/maxVelocity/jitter/field multipliers/particleSize (currently uses high exponents).

7) `SandLineMod`
- Migrate `pointRadius` and `density` to around-manual.

#### Priority B (heavily used / needs predictability)

8) `FadeMod`
- Convert half-life intent target mapping to around-manual so patches keep their tuned persistence identity.
- Keep inversion semantics, but bound the movement.

9) `PathMod`
- Already has custom capping behavior; revisit with around-manual only if it still causes large discontinuous shifts.

---

## Testing / validation

- Build key examples:
  - `addons/ofxMarkSynth/example_particles`
  - `addons/ofxMarkSynth/example_collage`
  - any fluid example used in performance

- Run config validators:
  - `addons/ofxMarkSynth/scripts/validate_synth_configs.py` against Improvisation1 configs

- Manual sanity pass (performer):
  - compare “Grain” and “Maximum” in a known-problem config (e.g. Improvisation1 config 26 wash layer)
  - ensure they remain expressive but don’t saturate/destroy the patch

---

## Documentation updates

- Add a short section to `docs/intent-implementation.md` describing “around-manual” mapping and why it exists.
- Update `docs/intent-mappings.md` entries for each migrated Mod to note which parameters use around-manual.

---

## Decision needed before coding Fluid dt

Choose default approach:
- A) Exclude `dt` from intent (preferred for safety)
- B) Keep `dt` intent-driven but explicitly couple Fluid + FluidRadialImpulse and document the tie
