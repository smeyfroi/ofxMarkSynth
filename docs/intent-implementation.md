# Intent System Implementation Guide

This document describes the Intent mechanism used by `ofxMarkSynth` Mods to respond to high-level artistic direction.

See also: [intent-mappings.md](intent-mappings.md) for per-Mod mapping reference.

---

## Overview

Intent is an abstraction layer that maps **artistic dimensions** (Energy, Density, Structure, Chaos, Granularity) to **Mod parameters**. Rather than controlling parameters directly, Intent expresses *what* the visual should feel like, and each Mod's `applyIntent()` method translates that into *how* its parameters should change.

## Key Components

| File | Purpose |
|------|---------|
| `Intent.hpp` | Defines the 5 Intent dimensions (E/D/S/C/G), each 0.0-1.0 |
| `IntentMapper.hpp` | Fluent API: `IntentMap` and `Mapping` classes |
| `IntentMapping.hpp` | Legacy helper functions (prefer `IntentMapper` for new code) |
| `ParamController.h` | Blends manual, auto (connections), and intent values |
| `Mod.hpp` | Base class with virtual `applyIntent()` method |

## Intent Dimensions

| Dimension | Accessor | Semantic Meaning |
|-----------|----------|------------------|
| Energy | `E()` | Speed, motion, intensity, magnitude |
| Density | `D()` | Quantity, opacity, detail level |
| Structure | `S()` | Organization, brightness, pattern regularity |
| Chaos | `C()` | Randomness, variance, noise, unpredictability |
| Granularity | `G()` | Scale, resolution, feature size |

All values are normalized to [0.0, 1.0].

---

## The Fluent API

### Basic Pattern

```cpp
void MyMod::applyIntent(const Intent& intent, float strength) {
  IntentMap im(intent);

  // Single dimension → parameter
  im.E().lin(radiusController, strength);
  im.D().exp(alphaController, strength);

  // With custom output range
  im.C().lin(noiseController, strength, 0.0f, 0.5f);
}
```

### `IntentMap` Class

Created from an `Intent`, provides dimension accessors that return `Mapping` objects:

```cpp
IntentMap im(intent);
Mapping energy = im.E();      // returns Mapping(intent.getEnergy(), "E")
Mapping density = im.D();     // returns Mapping(intent.getDensity(), "D")
```

### `Mapping` Class

Holds a value (0-1) and a label for debugging. Supports:

**Operators:**
- `*` — Multiply dimensions: `(im.E() * im.C())` yields product with label `"E*C"`
- `.inv()` — Inverse (1-value): `im.S().inv()` yields `1-S` with label `"1-S"`

**Terminal methods** (apply to a `ParamController`):
- `.lin(controller, strength)` — Linear map using controller's min/max
- `.lin(controller, strength, min, max)` — Linear map with custom range
- `.exp(controller, strength)` — Exponential (power=2.0) using controller's range
- `.exp(controller, strength, exponent)` — Exponential with custom power
- `.exp(controller, strength, min, max, exponent)` — Full control

**Raw access:**
- `.get()` — Returns the raw 0-1 value for manual composition
- `.getLabel()` — Returns the description string

### Mapping Functions Summary

| Method | Formula | Use Case |
|--------|---------|----------|
| `lin` | `lerp(min, max, v)` | Direct proportional control |
| `exp(n)` | `lerp(min, max, pow(v, n))` | More responsive at low values |
| `inv().lin` | `lerp(max, min, v)` | Inverse relationship |
| `inv().exp(n)` | `lerp(min, max, pow(1-v, n))` | Inverse with curve |

---

## Implementation Patterns

### Pattern 1: Simple Direct Mapping

Map one dimension to one parameter:

```cpp
void FadeMod::applyIntent(const Intent& intent, float strength) {
  IntentMap im(intent);
  im.D().exp(alphaMultiplierController, strength);
}
```

### Pattern 2: Dimension Combination

Multiply dimensions for compound effects:

```cpp
// Spin increases with both Energy and Chaos
(im.C() * im.E()).lin(spinController, strength, -0.05f, 0.05f);
```

### Pattern 3: Weighted Blend

Combine dimensions with explicit weights:

```cpp
// 80% Density, 20% Granularity
float combined = im.D().get() * 0.8f + im.G().get() * 0.2f;
float result = exponentialMap(combined, controller);
controller.updateIntent(result, strength, "D*.8+G*.2 -> exp");
```

### Pattern 4: Conditional/Discrete Values

Map dimensions to strategy enums or discrete choices:

```cpp
// Strategy selection based on Structure
int strategy;
float s = im.S().get();
if (s < 0.3f) strategy = 0;
else if (s < 0.7f) strategy = 1;
else strategy = 2;
strategyParameter.set(strategy);
```

### Pattern 5: Color Composition

Build colors from multiple dimensions:

```cpp
ofFloatColor color = energyToColor(intent);           // E → warm/cool
color.setBrightness(structureToBrightness(intent));   // S → brightness
color.setSaturation((im.E() * im.C() * im.S().inv()).get());  // composite
color.a = im.D().get();                               // D → alpha
colorController.updateIntent(color, strength, "E->color, S->bright, ...");
```

### Pattern 6: Inverse Relationships

Use `.inv()` for parameters that should decrease as a dimension increases:

```cpp
im.G().inv().lin(velocityDampingController, strength);  // High G → low damping
im.D().inv().lin(attractionRadiusController, strength); // High D → small radius
```

---

## ParamController Integration

Each parameter controlled by Intent needs a `ParamController<T>` that blends three value sources:

1. **Manual** — User's direct parameter edits (decays over time)
2. **Auto** — Values from Mod connections (`receive()` → `updateAuto()`)
3. **Intent** — Values from `applyIntent()` → `updateIntent()`

### Registering Controllers

In the Mod constructor:
```cpp
registerControllerForSource(radiusParameter, radiusController);
```

### Updating from `applyIntent()`

The fluent API calls `controller.updateIntent(value, strength, description)` automatically. For manual composition:

```cpp
controller.updateIntent(computedValue, strength, "D -> custom formula");
```

### The `strength` Parameter

- Passed to `applyIntent()` by the Synth
- Represents how strongly Intent should influence this update
- Always pass it to terminal methods or `updateIntent()`
- Enables smooth transitions between Intent presets

---

## AgencyFactor

Mods can scale their Intent responsiveness:

```cpp
float MyMod::getAgency() const {
  return Mod::getAgency() * agencyFactorParameter;
}
```

- `agencyFactorParameter` is typically a 0-1 parameter exposed in the GUI
- Setting it to 0 disables Intent for that Mod
- Setting it to 0.5 halves the Intent influence

---

## Helper Functions (IntentMapping.hpp)

For complex mappings not covered by the fluent API:

```cpp
// These return float values (not applied to controllers)
float linearMap(float intentValue, float minOut, float maxOut);
float exponentialMap(float intentValue, float minOut, float maxOut, float exponent = 2.0);
float inverseMap(float intentValue, float minOut, float maxOut);
float inverseExponentialMap(float intentValue, float minOut, float maxOut, float exponent = 2.0);

// Color helpers
ofFloatColor energyToColor(const Intent& intent);     // E → warm/cool palette
float structureToBrightness(const Intent& intent);    // S → 0.0-0.2
ofFloatColor densityToAlpha(const Intent& intent, const ofFloatColor& baseColor);
```

---

## Creating a New applyIntent() Implementation

1. **Identify parameters** that should respond to artistic direction
2. **Choose dimensions** based on semantic meaning (see table above)
3. **Select mapping functions** based on desired response curve
4. **Write mappings** using the fluent API
5. **Document** the mapping in [intent-mappings.md](intent-mappings.md)

### Template

```cpp
void MyMod::applyIntent(const Intent& intent, float strength) {
  IntentMap im(intent);

  // Direct mappings
  im.E().lin(paramAController, strength);
  im.D().exp(paramBController, strength, 2.0f);

  // Combined dimensions
  (im.E() * im.C()).lin(paramCController, strength, 0.0f, 1.0f);

  // Custom composition
  float custom = im.S().get() * 0.7f + im.G().get() * 0.3f;
  float result = exponentialMap(custom, paramDController);
  paramDController.updateIntent(result, strength, "S*.7+G*.3 -> exp");
}
```

---

## Debugging Tips

- Check `intentMappingDescription` in the GUI tooltip for active mappings
- Verify `hasReceivedIntentValue` is true after `applyIntent()` runs
- Use `getFormattedValue()` to see the auto/manual/intent breakdown
- Watch the Node Editor agency meters for visual feedback

---

**Document Version**: 1.0  
**Last Updated**: December 2025
