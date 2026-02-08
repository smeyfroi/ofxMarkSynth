# MarkSynth Recipes (Wiring + Tuning Cookbook)

This is a builder-facing cookbook of cross-Mod patterns.
It is intentionally generic (not tied to any specific performance folder’s presets).

Related:
- `docs/mods.md`: Mod reference
- `docs/performer-guide.md`: live performance controls

## Audio calibration (venue tuning)

When visuals feel dead or always-on, the first thing to check is `AudioDataSourceMod` normalization ranges.

Typical workflow:
- Set `Audio.MinRms` low enough that quiet passages register.
- Set `Audio.MaxRms` so loud passages don’t clamp.
- Tune `MinPitch`/`MaxPitch` only if pitch-based mappings are flat.

Prefer storing stable venue values in `venue-presets.json`.

## Untether quiet passages from the top edge (drift points)

If you use `Audio.PitchRmsPoint` directly, its Y value is normalized RMS, so very quiet audio tends to stick at `y≈0`.

Use drifted variants for spatial continuity:

- `Audio.DriftPitchRmsPoint` instead of `Audio.PitchRmsPoint`
- `Audio.DriftSpectral2dPoint` instead of `Audio.Spectral2dPoint`

Example:

```
Audio.DriftPitchRmsPoint -> Marks.Point
```

The drift sources:
- track the raw Y strongly when loud
- hold + drift + wrap when quiet

## RMS → layer persistence (recommended Fade pattern)

Use `FadeAlphaMap` to map a scalar (often `Audio.RmsScalar`) into `Fade.HalfLifeSec`.

```
Audio.RmsScalar -> FadeAlphaMap.float
FadeAlphaMap.float -> Fade.HalfLifeSec
```

Why:
- `FadeMod` uses a time-based half-life model (stable across frame rate)
- `FadeAlphaMap` is a compatibility bridge for older alpha-per-frame thinking

## Smear persistence uses HalfLifeSec (not AlphaMultiplier)

`SmearMod` also uses `HalfLifeSec` as the primary persistence control.

```
Audio.RmsScalar -> FadeAlphaMap.float
FadeAlphaMap.float -> Smear.HalfLifeSec
```

`AlphaMultiplier` still exists as a legacy sink/config key and is interpreted at 30fps.

## Field scaling: PreScaleExp × Multiplier

Several Mods interpret vector fields using a log-scale pre-normalization:
- `Smear.Field1PreScaleExp` + `Smear.Field1Multiplier`
- `Fluid.VelocityFieldPreScaleExp` + `Fluid.VelocityFieldMultiplier`
- `ParticleField.Field1PreScaleExp` + `ParticleField` multipliers (config-only)

Rule of thumb:
- `PreScaleExp` chooses an order-of-magnitude (because it is log10)
- `Multiplier` is the fine control

If a field effect is invisible:
- increase the multiplier first
- if it still does nothing, bump `PreScaleExp` by +1

## Video motion → fluid injection

```
VideoFlowSource.PointVelocity -> FluidRadialImpulse.PointVelocity
FluidRadialImpulse.Point -> Fluid.TempImpulsePoint (optional)
```

Notes:
- Start with `Impulse Strength` small and increase gradually.
- If it looks “dead”, check `VelocityScale` in `FluidRadialImpulse`.

## Palette stability for drones (SomPalette anti-collapse)

If the palette collapses or becomes too static under sustained tones:
- increase `SomPalette.AntiCollapseJitter` slightly
- ensure `SomPalette.WindowSecs` is long enough to capture change
- use `SomPalette.NoveltyEmitChance` to occasionally surface rare colors

If the palette changes too fast:
- increase `WindowSecs`
- decrease `TrainingStepsPerFrame`
- increase `ChipMemoryMultiplier`

## Geometric architecture from clustered points (DividedArea)

DividedArea works best when you split “big structure” and “detail”:

```
Audio.PolarPitchRmsPoint -> AudioClusters.Point
AudioClusters.ClusterCentre -> DividedArea.MajorAnchor

Audio.PolarPitchRmsPoint -> DividedArea.MinorAnchor
```

Notes:
- Feed `MajorAnchor` from a **batch** source (cluster centers) so major lines update coherently.
- Put major lines on a `major-lines` overlay layer if you want refraction/chromatic styles.

## Collage: path + snapshot + outline persistence

Core wiring:

```
Path.Path -> Collage.Path
PixelSnapshot.SnapshotTexture -> Collage.SnapshotTexture
SomPalette.Random -> Collage.Colour
```

Requirements:
- For snapshot strategies, the target drawing layer must have `useStencil: true`.

For outlines that linger:
- Draw outlines into a persistent `outlines` layer and apply a slow `Fade` to that layer.

## Glue Mod: MultiplyAdd as a scaler

`MultiplyAdd` maps `y = Multiplier*x + Adder`.

Common uses:
- Map a scalar into a radius/strength range.
- Convert event magnitudes into stable parameter ranges.

Example:

```
Audio.SpectralCentroidScalar -> SomeMap.float
SomeMap.float -> Accents.Radius
```

Practical tip: keep `Multiplier` small for parameters that are sensitive near 0 (alpha, impulses).
