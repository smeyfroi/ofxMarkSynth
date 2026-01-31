# Wrapper Mod ParamControllers (External Implementations)

Note: deferred/backlog items live in `docs/future-work.md`.

This document describes the agreed pattern and a complete implementation plan for Mods that **wrap external implementations** (e.g. `FluidMod` wrapping `ofxRenderer::FluidSimulation`).

## Problem statement

`ofxMarkSynth::ParamController<T>` blends three influences:
- Manual UI/config value (`ofParameter<T>`)
- Auto values from graph connections (`updateAuto`)
- Intent-driven values (`updateIntent`)

`ParamController<T>` exposes the blended result as `controller.value`, but it **does not automatically apply** that value to anything.

For internal Mods (where code uses `controller.value` directly), this is fine.

For wrapper Mods, the wrapped implementation often reads its own `ofParameter<T>::get()` values internally. In that case:
- Manual/config changes affect behavior
- Auto/intent blending does **not** affect behavior unless the wrapper explicitly forwards the blended values

We also must not “write back” the controller value into the manual `ofParameter<T>` because the controller’s manual-touch detection is currently based on parameter listeners; programmatic writes look like user manipulation and distort the weight logic.

## Decisions (locked)

- Use **Option 1 only**: controllers compute effective values; wrappers push those values into the external implementation via an explicit API.
- External implementations will accept overrides via `std::optional` fields.
- Wrapper and engine should both **debounce** to avoid triggering work when nothing changed.
- Avoid `git restore` during implementation work.

## Terminology

- **Manual parameter**: an `ofParameter<T>` that is shown in the GUI and serialized in config snapshots.
- **Effective value**: the blended output (`ParamController<T>::value`) that should drive the runtime behavior.
- **Override**: an effective value pushed into the external implementation (without touching manual parameters).

## Pattern: manual params + parameter overrides

### External implementation responsibilities

Each external implementation that supports controller-driven behavior should provide:

1) An `ofParameterGroup` for manual/config/UI values (existing behavior).

2) A lightweight override struct:

```cpp
struct ParameterOverrides {
  std::optional<float> foo;
  std::optional<int> bar;
  // ... only what we want controllable
};
```

3) A stable API:

```cpp
void setParameterOverrides(const ParameterOverrides& overrides);
void clearParameterOverrides(); // optional but recommended
```

4) A way to read **effective** values internally:

```cpp
float getFooEffective() const {
  return overrides.foo.value_or(fooParameter.get());
}
```

5) A debounce mechanism:
- If new overrides are equal to current overrides (field-by-field), do nothing.
- For float comparisons, prefer explicit rounding/clamping in the wrapper so equality stays stable.

6) If the external implementation is threaded or has expensive precomputation, the override setter must only trigger work when needed.

### Wrapper Mod responsibilities

For each controllable parameter:

1) Bind a `ParamController<T>` to the manual `ofParameter<T>`.

2) In `update()`:
- `syncControllerAgencies()`
- `controller.update()`

3) Build an overrides struct using **effective** values:

```cpp
External::ParameterOverrides overrides;
overrides.foo = controllerFoo.value;
```

4) Debounce in the Mod:
- Cache the last applied overrides (or a reduced “key” representation).
- Only call `external.setParameterOverrides(...)` when changed.

5) Never assign effective values back into manual `ofParameter` objects.

## Debounce guidelines

Use the cheapest stable representation available:

- Prefer rounding/clamping effective floats to the actual runtime type:
  - Example: cluster count should be `int` (use `std::lround` + clamp) before putting into overrides.
- Prefer exact equality on integers/enums.
- For floats that truly must remain float:
  - choose an explicit quantization step (e.g. `0.0001f`) and store the quantized value in overrides
  - or compare with an epsilon in both wrapper and engine (but be consistent)

Debounce should exist in both places:

- **Wrapper debounce** prevents redundant calls from the graph update loop.
- **Engine debounce** prevents redundant internal work if a wrapper accidentally spams overrides.

## Threading / expensive work rules

When an external system runs on a thread or triggers expensive recompute:

- `setParameterOverrides()` must not start new threads.
- It may signal existing worker threads using an existing channel/condition variable.
- It must only signal when the relevant override fields actually changed.

If an external system currently only recomputes on “new input” events, override updates that affect computation must also trigger recompute.

## Scope discovery: which Mods are wrappers?

Current heuristic (works well in this repo):

- Mods that call `addFlattenedParameterGroup(parameters, someExternal.getParameterGroup())`
- Mods that hold a non-trivial external instance (e.g. `FluidSimulation`, `PointClusters`, `DividedArea`, `ParticleSet`, etc.)

Currently identified wrapper Mods in `ofxMarkSynth` that are in scope for Intent + overrides:

- `src/layerMods/FluidMod.*` (wraps `ofxRenderer::FluidSimulation`)
- `src/processMods/ClusterMod.*` (wraps `ofxPointClusters::PointClusters`)
- `src/sinkMods/DividedAreaMod.*` (wraps `ofxDividedArea::DividedArea`)
- `src/sinkMods/ParticleSetMod.*` (wraps `ofxParticleSet::ParticleSet`)
- `src/sinkMods/ParticleFieldMod.*` (wraps `ofxParticleField` / `ParticleField`)

Wrapper Mods explicitly out of scope for Intent (manual-only, no overrides work planned):

- `src/sourceMods/VideoFlowSourceMod.*`
- `src/sourceMods/AudioDataSourceMod.*`

This list should be kept current as new wrapper Mods are added.

## Repo-wide implementation plan

### Phase 0 — Audit and acceptance criteria

1) Enumerate every wrapper Mod and list:
- Which manual params should be controllable (auto + intent)
- Which params should remain “manual only” (for safety or to avoid instability)

2) For each wrapper Mod, define the acceptance criteria:
- Which parameters must respond to controller blending (visual sanity)
- What counts as “no extra work” (debounce)
- Threading invariants (no thread churn)

### Phase 1 — Introduce a common overrides convention

1) For each external library, add a `ParameterOverrides` struct with `std::optional` fields.

2) Add `setParameterOverrides(...)` + engine-side debounce.

3) Update the engine to consume “effective” values consistently:
- Replace internal reads of `param.get()` with `getXEffective()` in the computation path.

4) For threaded engines, add a minimal “poke” signal so override updates can trigger recompute when necessary.

### Phase 2 — Update wrapper Mods to apply overrides

For each wrapper Mod:

1) Create controllers bound to manual params (existing pattern).

2) Update each frame:
- update controllers
- build overrides from controller outputs
- apply overrides **only when changed**

3) Remove any existing programmatic writes to manual `ofParameter` that are only there to push effect values.

### Phase 3 — Validation / test passes

This repo has no automated tests for this behavior; validation is via:

- Build affected examples
- Interactive sanity checks (GUI + intent + connections)
- Config validator runs where applicable

For each wrapper, choose the minimal example app that exercises it.

### Phase 4 — Documentation / config guidance

- Update `docs/mods.md` where necessary to clarify which parameters are manual-only vs controllable.
- If any config keys change or new invariants are introduced, update `docs/synth-config-reference.json`.

## Per-implementation plans

### 1) `ofxRenderer::FluidSimulation` (via `FluidMod`)

Files:
- `src/layerMods/FluidMod.cpp`
- `src/layerMods/FluidMod.hpp`
- `ofxRenderer/src/fluid/FluidSimulation.h` (external repo)

Plan:

1) Add `FluidSimulation::ParameterOverrides` in ofxRenderer, with optional fields for the subset we want controller-driven (initially: `dt`, `Vorticity`, `Value Dissipation`, `Velocity Dissipation`, plus any already-used temperature impulse knobs if needed).

2) Implement `setParameterOverrides()` with debounce.

3) Ensure `FluidSimulation::update()` uses effective values (override-or-manual).

4) In `FluidMod::update()`:
- compute overrides from controller outputs
- debounce (cache last applied overrides)
- call `fluidSimulation.setParameterOverrides(overrides)`

5) Validate with `example_simple` and any host app that uses fluid.

Notes:
- Maintain existing strict validation rules (wrap/boundary/dt invariants). Overrides must not bypass validation; they should feed into the same validation/compute path.

### 2) `ofxPointClusters::PointClusters` (via `ClusterMod`)

Files:
- `src/processMods/ClusterMod.cpp`
- `ofxPointClusters/src/ofxPointClusters.h` and `.cpp`

Current gotcha:
- Clustering recompute is triggered only by thread channel updates (new points / pruning). Changing cluster count without new points does not recompute.

Plan:

1) Add `PointClusters::ParameterOverrides`:
- `std::optional<int> numClusters`
- optionally `std::optional<int> maxSourcePoints`

2) Add `setParameterOverrides()` with debounce.

3) When `numClusters` changes, signal the existing worker thread to recompute:
- send a lightweight “control update” message on the existing channel or a new channel
- ensure recompute happens even when no new points arrive

4) Update `PointClusters` internal reads:
- use effective cluster count in `updateClusters()`

5) Update `ClusterMod::update()`:
- compute `effectiveNumClusters = clamp(round(controller.value))`
- apply via `pointClusters.setParameterOverrides({ .numClusters = effectiveNumClusters })` with debounce

### 3) `ofxDividedArea::DividedArea` (via `DividedAreaMod`)

Files:
- `src/sinkMods/DividedAreaMod.cpp`
- `src/sinkMods/DividedAreaMod.hpp`
- `ofxDividedArea` addon source (external repo)

Current observations:
- Some values are already applied directly (e.g. `maxUnconstrainedDividerLines` is set from a controller).
- Some values appear to be controlled only via `ofParameter` (e.g. `unconstrainedSmoothnessParameter`), which likely won’t receive controller blending unless overrides exist.
- There are some manual parameter assignments in response to sinks (e.g. setting `angleParameter` after `updateAuto`). These should be reviewed and ideally removed in favor of overrides.

Plan:

1) Add `DividedArea::ParameterOverrides` for any parameters that are used in the internal algorithm and should respond to controllers.

2) Implement `setParameterOverrides()` with debounce; apply effective values in compute path.

3) Update `DividedAreaMod`:
- Convert direct field writes into overrides where appropriate (or keep direct field writes if they are not `ofParameter` based and already safe).
- Remove programmatic writes to manual `ofParameter` that exist only to push effective values.

4) Validate in the relevant example app(s).

### 4) `ofxParticleSet::ParticleSet` (via `ParticleSetMod`)

Files:
- `src/sinkMods/ParticleSetMod.cpp`
- external `ofxParticleSet` addon

Current observation:
- Controllers are created for parameters that are likely owned by `ParticleSet`, but `ParticleSet::update()` probably reads its own `ofParameter.get()` values.

Plan:

1) Add `ParticleSet::ParameterOverrides` for the parameters that have controllers in `ParticleSetMod`:
- `timeStep`, `velocityDamping`, `attractionStrength`, etc.

2) Implement `setParameterOverrides()` with debounce.

3) Update `ParticleSet` compute to use effective values.

4) Update `ParticleSetMod::update()`:
- compute overrides from controller outputs
- apply overrides with debounce

### 5) `ParticleField` (via `ParticleFieldMod`)

Files:
- `src/sinkMods/ParticleFieldMod.cpp`
- external addon that provides `particleField.getParameterGroup()`

Plan mirrors ParticleSet:

1) Add `ParameterOverrides` + `setParameterOverrides()` to the external `ParticleField` class.

2) Apply overrides in compute path.

3) Update `ParticleFieldMod` to apply overrides with debounce.

### 6) Out of scope: video + audio source Mods

The following Mods wrap external implementations, but they will **never** participate in Intent-driven control, so no `ParameterOverrides` API work is planned for them.

- `src/sourceMods/VideoFlowSourceMod.*`: keep `PointSamplesPerUpdate` as an internal controller (already uses `controller.value`), and treat wrapped `MotionFromVideo` parameters as manual-only.
- `src/sourceMods/AudioDataSourceMod.*`: treat audio processor tuning parameters as manual-only.

## Suggested helper utilities (optional)

If the amount of boilerplate becomes significant, add a small helper in `ofxMarkSynth` (not in the external libraries) to standardize:

- Creating controllers for a set of manual params
- Building an overrides struct
- Debouncing and applying

This is a quality-of-life improvement; it should not obscure the explicitness of the override application.

## Manual validation checklist

For each wrapper Mod:

- Build the most relevant example app.
- Confirm manual slider changes still work.
- Add a graph connection to drive the parameter (auto); confirm the parameter responds.
- Trigger intent changes; confirm the parameter responds.
- Confirm “manual touch” bias still behaves correctly (no false manual touches from programmatic writes).
- Confirm CPU/GPU load does not spike when steady-state (debounce working).
- For threaded implementations, confirm no thread churn and that override changes take effect even without new input events.
