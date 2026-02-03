#!/usr/bin/env python3

"""Intent impact analysis + artifact generation (UI + HTML + optional Markdown).

This script centralizes the per-performance intent impact analysis.

Key decisions (current):
- Baseline = "no intent" => each parameter's manual config value, falling back to implementation defaults.
- Row schema = 4 mark types (stable keys): Motion / Particles / Marks / Geometry.
- Only consider ofParameter-backed values for baselines/defaults.
- Ignore colour vectors.
- Consider vec magnitude ONLY for vec params whose name contains "Spread".

Outputs:
- Update `intents[].ui.impact` inside configs (optional)
- Compact single-file HTML grid (optional)
- Markdown impact tables (optional; for audit)

Usage (example):
  python3 scripts/intent_impact.py \
    --configs-dir /path/to/config/synth \
    --write-ui \
    --out-html /path/to/intent-impact-grid.html

"""

from __future__ import annotations

import argparse
import glob
import html
import json
import math
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable


ROW_KEYS = ["Motion", "Particles", "Marks", "Geometry"]

# Mark-type row mapping.
ROW_MOTION = {"Fluid", "FluidRadialImpulse", "VideoFlowSource", "Smear"}
ROW_PARTICLES = {"ParticleSet", "ParticleField"}
ROW_MARKS = {"SoftCircle", "SandLine", "Text"}
ROW_GEOMETRY = {"DividedArea", "Collage", "Path"}

MOD_IS_FADE = {"Fade"}

# Mods to ignore in impact scoring.
OMIT_MOD_TYPES = {
    "AudioDataSource",
    "SomPalette",
    "MultiplyAdd",
    "VectorMagnitude",
    "RandomVecSource",
    "RandomHslColor",
    "StaticTextSource",
    "TextSource",
}


@dataclass(frozen=True)
class Intent:
    name: str
    E: float
    D: float
    S: float
    C: float
    G: float


@dataclass(frozen=True)
class ParamSpec:
    default: float
    min_val: float
    max_val: float


@dataclass(frozen=True)
class ProxyParam:
    param_name: str
    weight: float


def clamp01(x: float) -> float:
    return max(0.0, min(1.0, x))


def normalize(v: float, spec: ParamSpec) -> float:
    if spec.max_val <= spec.min_val:
        return 0.5
    return clamp01((v - spec.min_val) / (spec.max_val - spec.min_val))


def exp_norm(x: float, exponent: float = 2.0) -> float:
    return clamp01(x) ** exponent


def inv_norm(x: float) -> float:
    return 1.0 - clamp01(x)


def inv_exp_norm(x: float, exponent: float = 2.0) -> float:
    return (1.0 - clamp01(x)) ** exponent


def parse_float(value: object) -> float | None:
    if value is None:
        return None
    if isinstance(value, (int, float)):
        return float(value)
    if not isinstance(value, str):
        return None
    s = value.strip().replace("f", "")
    if not s:
        return None
    try:
        return float(s)
    except ValueError:
        return None


def parse_vec2_mag(value: object) -> float | None:
    if not isinstance(value, str):
        return None
    parts = [p.strip() for p in value.split(",")]
    nums = [parse_float(p) for p in parts]
    nums = [n for n in nums if n is not None]
    if len(nums) < 2:
        return None
    return float(math.sqrt(nums[0] * nums[0] + nums[1] * nums[1]))


def row_for_mod_type(mod_type: str) -> str | None:
    if mod_type in ROW_MOTION:
        return "Motion"
    if mod_type in ROW_PARTICLES:
        return "Particles"
    if mod_type in ROW_MARKS:
        return "Marks"
    if mod_type in ROW_GEOMETRY:
        return "Geometry"
    return None


def mod_layer_names(mod: dict) -> set[str]:
    names: set[str] = set()
    layers = mod.get("layers")
    if not isinstance(layers, dict):
        return names
    for v in layers.values():
        if isinstance(v, list):
            for name in v:
                if isinstance(name, str):
                    names.add(name)
    return names


def impact_to_int(cell: str) -> int:
    s = cell.strip()
    if not s:
        return 0
    plus = s.count("+")
    minus = s.count("-")
    if plus and minus:
        return 0
    if plus:
        return min(3, plus)
    if minus:
        return -min(3, minus)
    return 0


def rate_delta(delta: float) -> int:
    a = abs(delta)
    if a < 0.05:
        return 0
    if a < 0.15:
        return 1 if delta > 0 else -1
    if a < 0.30:
        return 2 if delta > 0 else -2
    return 3 if delta > 0 else -3


def parse_ofparameter_specs(paths: Iterable[Path]) -> dict[str, ParamSpec]:
    """Parse ofParameter<T> { "Name", default, min, max } into ParamSpec.

    Supports float/int/bool (bool will be skipped because min/max is not meaningful here).
    Also supports vec2-like params, but we will only use them when the name contains "Spread".
    """

    specs: dict[str, ParamSpec] = {}

    float_int_pat = re.compile(
        r"ofParameter<\s*(?P<type>float|int)\s*>\s+\w+\s*\{\s*\"(?P<name>[^\"]+)\"\s*,\s*(?P<default>[^,}]+)\s*,\s*(?P<min>[^,}]+)\s*,\s*(?P<max>[^}]+)\s*\}\s*;"
    )

    # e.g. ofParameter<glm::vec2> foo { "Velocity Spread", glm::vec2{0.2,0.2}, glm::vec2{0,0}, glm::vec2{1,1} };
    vec2_pat = re.compile(
        r"ofParameter<\s*(?:glm::vec2|ofVec2f)\s*>\s+\w+\s*\{\s*\"(?P<name>[^\"]+)\"\s*,\s*(?P<default>[^,}]+\}\s*)\s*,\s*(?P<min>[^,}]+\}\s*)\s*,\s*(?P<max>[^}]+\}\s*)\s*\}\s*;"
    )

    def to_number(expr: str) -> float | None:
        s = expr.strip().replace("f", "")
        try:
            return float(s)
        except ValueError:
            return None

    def to_vec_mag(expr: str) -> float | None:
        nums = [float(x) for x in re.findall(r"-?\d+(?:\.\d+)?", expr)]
        if len(nums) < 2:
            return None
        return float(math.sqrt(nums[0] * nums[0] + nums[1] * nums[1]))

    for p in paths:
        if not p.exists():
            continue
        text = p.read_text(encoding="utf-8", errors="replace")

        for m in float_int_pat.finditer(text):
            name = m.group("name")
            d = to_number(m.group("default"))
            mn = to_number(m.group("min"))
            mx = to_number(m.group("max"))
            if d is None or mn is None or mx is None:
                continue
            specs[name] = ParamSpec(default=d, min_val=mn, max_val=mx)

        for m in vec2_pat.finditer(text):
            name = m.group("name")
            # Only include vec2 params that look like spread controls.
            if "spread" not in name.lower():
                continue
            d = to_vec_mag(m.group("default"))
            mn = to_vec_mag(m.group("min"))
            mx = to_vec_mag(m.group("max"))
            if d is None or mn is None or mx is None:
                continue
            specs[name] = ParamSpec(default=d, min_val=mn, max_val=mx)

    return specs


def build_param_specs(of_root: Path) -> dict[str, dict[str, ParamSpec]]:
    """Return modType -> paramName -> ParamSpec."""

    addons = of_root / "addons"
    ofx_mark = addons / "ofxMarkSynth"

    def p(*parts: str) -> Path:
        return Path(*parts)

    # ofxMarkSynth Mod headers
    src = ofx_mark / "src"

    specs: dict[str, dict[str, ParamSpec]] = {}

    # Wrapped: ofxRenderer FluidSimulation
    specs["Fluid"] = parse_ofparameter_specs(
        [addons / "ofxRenderer" / "src" / "fluid" / "FluidSimulation.h"]
    )

    # MarkSynth-defined params
    specs["FluidRadialImpulse"] = parse_ofparameter_specs(
        [src / "sinkMods" / "FluidRadialImpulseMod.hpp"]
    )
    specs["Smear"] = parse_ofparameter_specs([src / "layerMods" / "SmearMod.hpp"])
    specs["Fade"] = parse_ofparameter_specs([src / "layerMods" / "FadeMod.hpp"])
    specs["SoftCircle"] = parse_ofparameter_specs(
        [src / "sinkMods" / "SoftCircleMod.hpp"]
    )
    specs["SandLine"] = parse_ofparameter_specs([src / "sinkMods" / "SandLineMod.hpp"])
    specs["Text"] = parse_ofparameter_specs([src / "sinkMods" / "TextMod.hpp"])
    specs["Path"] = parse_ofparameter_specs([src / "processMods" / "PathMod.hpp"])
    specs["Collage"] = parse_ofparameter_specs([src / "sinkMods" / "CollageMod.hpp"])

    # Wrapped: ofxParticleSet / ofxParticleField
    specs["ParticleSet"] = parse_ofparameter_specs(
        [addons / "ofxParticleSet" / "src" / "ofxParticleSet.h"]
    )
    specs["ParticleField"] = parse_ofparameter_specs(
        [addons / "ofxParticleField" / "src" / "ParticleField.h"]
    )

    # Wrapped: ofxDividedArea
    # DividedAreaMod wraps multiple param groups; parse both.
    specs["DividedArea"] = parse_ofparameter_specs(
        [
            addons / "ofxDividedArea" / "src" / "ofxDividedArea.h",
            addons / "ofxDividedArea" / "src" / "MajorLineShaders.h",
            src / "sinkMods" / "DividedAreaMod.hpp",
        ]
    )

    # Wrapped: ofxMotionFromVideo
    specs["VideoFlowSource"] = parse_ofparameter_specs(
        [
            src / "sourceMods" / "VideoFlowSourceMod.hpp",
            addons / "ofxMotionFromVideo" / "src" / "ofxMotionFromVideo.h",
        ]
    )

    # Wrapped: ofxPointClusters
    specs["Cluster"] = parse_ofparameter_specs(
        [
            addons / "ofxPointClusters" / "src" / "ofxPointClusters.h",
            src / "processMods" / "ClusterMod.hpp",
        ]
    )

    return specs


def read_intents(config: dict) -> list[Intent]:
    intents: list[Intent] = []
    for it in config.get("intents", []):
        if not isinstance(it, dict):
            continue
        name = it.get("name")
        if not isinstance(name, str) or not name:
            continue
        intents.append(
            Intent(
                name=name,
                E=float(it.get("energy", 0.0)),
                D=float(it.get("density", 0.0)),
                S=float(it.get("structure", 0.0)),
                C=float(it.get("chaos", 0.0)),
                G=float(it.get("granularity", 0.0)),
            )
        )
    return intents


def proxies_for_mod_type(mod_type: str) -> list[ProxyParam]:
    # Only include ofParameter-backed values; ignore colour.

    if mod_type == "Fluid":
        return [
            ProxyParam("dt", 0.9),
            ProxyParam("Vorticity", 0.9),
            ProxyParam("Value Dissipation", 0.6),
            ProxyParam("Velocity Dissipation", 0.6),
        ]

    if mod_type == "FluidRadialImpulse":
        return [
            ProxyParam("Impulse Radius", 0.7),
            ProxyParam("Impulse Strength", 1.0),
            ProxyParam("SwirlStrength", 0.7),
            ProxyParam("SwirlVelocity", 0.7),
        ]

    if mod_type == "VideoFlowSource":
        return [ProxyParam("power", 1.0)]

    if mod_type == "Smear":
        return [
            ProxyParam("MixNew", 0.8),
            ProxyParam("AlphaMultiplier", 0.9),
            ProxyParam("Field2Multiplier", 0.6),
        ]

    if mod_type == "ParticleSet":
        return [
            ProxyParam("timeStep", 0.6),
            ProxyParam("forceScale", 0.6),
            ProxyParam("maxSpeed", 0.6),
            ProxyParam("connectionRadius", 0.4),
            ProxyParam("velocityDamping", 0.4),
        ]

    if mod_type == "ParticleField":
        return [
            ProxyParam("ln2ParticleCount", 0.4),
            ProxyParam("forceMultiplier", 0.7),
            ProxyParam("maxVelocity", 0.7),
            ProxyParam("jitterStrength", 0.6),
            ProxyParam("particleSize", 0.6),
        ]

    if mod_type == "SoftCircle":
        return [
            ProxyParam("Radius", 0.8),
            ProxyParam("AlphaMultiplier", 0.8),
            ProxyParam("Softness", 0.3),
        ]

    if mod_type == "SandLine":
        return [
            ProxyParam("Density", 0.8),
            ProxyParam("PointRadius", 0.6),
            ProxyParam("StdDevPerpendicular", 0.3),
            ProxyParam("StdDevAlong", 0.3),
        ]

    if mod_type == "Text":
        return [
            ProxyParam("Alpha", 0.8),
            ProxyParam("FontSize", 0.4),
        ]

    if mod_type == "DividedArea":
        return [
            ProxyParam("MaxUnconstrainedLines", 0.6),
            ProxyParam("MajorLineWidth", 0.6),
            ProxyParam("maxConstrainedLines", 0.4),
        ]

    if mod_type == "Collage":
        return [
            ProxyParam("Opacity", 0.8),
            ProxyParam("OutlineAlphaFactor", 0.6),
        ]

    if mod_type == "Path":
        return [
            ProxyParam("MaxVertices", 0.5),
            ProxyParam("ClusterRadius", 0.6),
        ]

    if mod_type == "Fade":
        return [ProxyParam("Alpha", 1.0)]

    return []


def target_norm_for_proxy(mod_type: str, proxy: ProxyParam, intent: Intent) -> float:
    # Intent mapping heuristics in normalized space.

    name = proxy.param_name

    if mod_type == "Fluid":
        if name == "dt":
            return exp_norm(intent.E, 2.0)
        if name == "Vorticity":
            return clamp01(intent.C * (1.0 - 0.75 * intent.S))
        if name == "Value Dissipation":
            return inv_norm(intent.D)
        if name == "Velocity Dissipation":
            return inv_exp_norm(intent.G, 2.0)

    if mod_type == "FluidRadialImpulse":
        structure = clamp01(intent.S)
        if name == "Impulse Radius":
            return clamp01(exp_norm(intent.G, 2.0) * (1.0 - 0.4 * structure))
        if name == "Impulse Strength":
            combined = clamp01(intent.E * 0.8 + intent.C * 0.2)
            return clamp01(exp_norm(combined, 4.0) * (1.0 - 0.7 * structure))
        if name in ("SwirlStrength", "SwirlVelocity"):
            swirl_dim = clamp01(intent.C * (1.0 - 0.7 * structure))
            return exp_norm(swirl_dim, 2.0)

    if mod_type == "VideoFlowSource":
        if name == "power":
            return exp_norm(intent.E, 2.0)

    if mod_type == "Smear":
        if name == "MixNew":
            return exp_norm(intent.E, 2.0)
        if name == "AlphaMultiplier":
            return exp_norm(intent.D, 2.0)
        if name == "Field2Multiplier":
            return exp_norm(intent.C, 3.0)

    if mod_type == "ParticleSet":
        if name == "timeStep":
            return exp_norm(intent.E, 2.0)
        if name == "forceScale":
            return clamp01(intent.E)
        if name == "maxSpeed":
            return clamp01(intent.C)
        if name == "connectionRadius":
            return clamp01(intent.D)
        if name == "velocityDamping":
            return inv_norm(intent.G)

    if mod_type == "ParticleField":
        if name == "ln2ParticleCount":
            return clamp01(intent.D)
        if name == "forceMultiplier":
            return exp_norm(intent.E, 2.0)
        if name == "maxVelocity":
            return clamp01(intent.E)
        if name == "jitterStrength":
            return clamp01(intent.C)
        if name == "particleSize":
            return exp_norm(intent.G, 2.0)

    if mod_type == "SoftCircle":
        if name == "Radius":
            return exp_norm(intent.E, 2.0)
        if name == "AlphaMultiplier":
            return clamp01(intent.D)
        if name == "Softness":
            return clamp01(intent.S)

    if mod_type == "SandLine":
        if name == "Density":
            return exp_norm(clamp01(intent.E * intent.G), 2.0)
        if name == "PointRadius":
            return exp_norm(intent.G, 3.0)
        if name == "StdDevPerpendicular":
            return clamp01(intent.C)
        if name == "StdDevAlong":
            return inv_norm(intent.S)

    if mod_type == "Text":
        if name == "Alpha":
            return clamp01(intent.D)
        if name == "FontSize":
            return exp_norm(intent.G, 2.0)

    if mod_type == "DividedArea":
        if name == "MaxUnconstrainedLines":
            return exp_norm(intent.C, 2.0)
        if name == "MajorLineWidth":
            return exp_norm(intent.G, 2.0)
        if name == "maxConstrainedLines":
            return clamp01(intent.D)

    if mod_type == "Collage":
        if name == "Opacity":
            return clamp01(intent.D)
        if name == "OutlineAlphaFactor":
            return exp_norm(intent.S, 2.0)

    if mod_type == "Path":
        if name == "MaxVertices":
            return exp_norm(intent.C, 2.0)
        if name == "ClusterRadius":
            return exp_norm(intent.G, 2.0)

    if mod_type == "Fade":
        if name == "HalfLifeSec":
            # Fade half-life behaves like persistence control; tie to density+granularity.
            # Higher density/granularity => faster fade => smaller half-life.
            return inv_exp_norm(clamp01(intent.D * 0.8 + intent.G * 0.2), 2.0)

    return 0.0


def get_baseline_value(mod: dict, proxy: ProxyParam, spec: ParamSpec) -> float:
    cfg = mod.get("config")
    if isinstance(cfg, dict) and proxy.param_name in cfg:
        v = parse_float(cfg.get(proxy.param_name))
        if v is not None:
            return v

    # Spread vec magnitude only (name contains Spread); allow config vec2 strings.
    if (
        isinstance(cfg, dict)
        and "spread" in proxy.param_name.lower()
        and proxy.param_name in cfg
    ):
        mag = parse_vec2_mag(cfg.get(proxy.param_name))
        if mag is not None:
            return mag

    return spec.default


def score_config(
    config: dict,
    intents: list[Intent],
    specs_by_type: dict[str, dict[str, ParamSpec]],
) -> dict[str, dict[str, int]]:
    """Return row -> intentName -> impact int [-3..+3]"""

    # Collect mod instances by row
    row_mods: dict[str, list[tuple[str, dict]]] = {k: [] for k in ROW_KEYS}
    fade_mods: list[dict] = []

    mods = config.get("mods") or {}
    if not isinstance(mods, dict):
        return {k: {} for k in ROW_KEYS}

    for mod in mods.values():
        if not isinstance(mod, dict):
            continue
        mod_type = mod.get("type")
        if not isinstance(mod_type, str):
            continue
        if mod_type in OMIT_MOD_TYPES:
            continue
        if mod_type in MOD_IS_FADE:
            fade_mods.append(mod)
            continue

        row = row_for_mod_type(mod_type)
        if row is None:
            continue
        row_mods[row].append((mod_type, mod))

    # Apply Fade as modifier: assign each Fade to any row that has a mod writing to the same layer.
    if fade_mods:
        layer_to_rows: dict[str, set[str]] = {}
        for row, mods_list in row_mods.items():
            for _t, m in mods_list:
                for lname in mod_layer_names(m):
                    layer_to_rows.setdefault(lname, set()).add(row)

        for fade in fade_mods:
            targets = set()
            for lname in mod_layer_names(fade):
                targets |= layer_to_rows.get(lname, set())
            for row in targets:
                row_mods[row].append(("Fade", fade))

    results: dict[str, dict[str, int]] = {k: {} for k in ROW_KEYS}

    for row, mods_list in row_mods.items():
        if not mods_list:
            for intent in intents:
                results[row][intent.name] = 0
            continue

        # Compute baseline intensity as weighted mean of normalized proxy values.
        base_vals: list[float] = []
        base_w: list[float] = []

        for mod_type, mod in mods_list:
            proxies = proxies_for_mod_type(mod_type)
            if not proxies:
                continue
            specs = specs_by_type.get(mod_type, {})

            for proxy in proxies:
                spec = specs.get(proxy.param_name)
                if spec is None:
                    continue
                manual = get_baseline_value(mod, proxy, spec)
                base_vals.append(normalize(manual, spec))
                base_w.append(proxy.weight)

        if not base_w:
            for intent in intents:
                results[row][intent.name] = 0
            continue

        baseline = sum(v * w for v, w in zip(base_vals, base_w)) / sum(base_w)

        for intent in intents:
            t_vals: list[float] = []
            t_w: list[float] = []

            for mod_type, mod in mods_list:
                proxies = proxies_for_mod_type(mod_type)
                if not proxies:
                    continue
                specs = specs_by_type.get(mod_type, {})

                for proxy in proxies:
                    spec = specs.get(proxy.param_name)
                    if spec is None:
                        continue
                    t_vals.append(target_norm_for_proxy(mod_type, proxy, intent))
                    t_w.append(proxy.weight)

            if not t_w:
                results[row][intent.name] = 0
                continue

            target = sum(v * w for v, w in zip(t_vals, t_w)) / sum(t_w)
            results[row][intent.name] = rate_delta(target - baseline)

    return results


def ensure_ui_written(config: dict, impacts: dict[str, dict[str, int]]) -> bool:
    intents = config.get("intents")
    if not isinstance(intents, list):
        return False

    # impacts[row][intentName] -> int
    by_name: dict[str, dict] = {}
    for it in intents:
        if not isinstance(it, dict):
            continue
        name = it.get("name")
        if not isinstance(name, str) or not name:
            continue
        by_name[name] = it

    changed = False

    for intent_name, intent_obj in by_name.items():
        if not isinstance(intent_obj, dict):
            continue

        ui = intent_obj.get("ui")
        if not isinstance(ui, dict):
            ui = {}
            intent_obj["ui"] = ui
            changed = True

        impact_obj = ui.get("impact")
        if not isinstance(impact_obj, dict):
            impact_obj = {}

        next_impact = {row: int(impacts[row].get(intent_name, 0)) for row in ROW_KEYS}

        if impact_obj != next_impact:
            ui["impact"] = next_impact
            changed = True

    return changed


def strip_ui_from_config(config: dict) -> bool:
    intents = config.get("intents")
    if not isinstance(intents, list):
        return False

    changed = False
    for it in intents:
        if not isinstance(it, dict):
            continue
        if "ui" in it:
            it.pop("ui", None)
            changed = True

    return changed


def abbreviate_row(row: str) -> str:
    return row[:4]


def impact_class(v: int) -> str:
    v = max(-3, min(3, int(v)))
    if v == 0:
        return "z0"
    return f"p{v}" if v > 0 else f"n{abs(v)}"


def render_intent_key(intents: list[str]) -> str:
    items = "".join(
        f'<span class="item"><span class="col"></span><code>{html.escape(name)}</code></span>'
        for name in intents
    )
    return f'<div class="intent-key inline">{items}</div>'


def write_html(
    out_path: Path,
    title: str,
    blocks: list[dict[str, Any]],
) -> None:
    # blocks elements: {idx, filename, desc, x,y, intents, impacts[row][intent] int}

    max_x = max((b.get("x") for b in blocks if b.get("x") is not None), default=None)
    max_y = max((b.get("y") for b in blocks if b.get("y") is not None), default=None)
    use_grid = max_x is not None and max_y is not None

    css = """
:root {
  --bg: #111;
  --panel: #1b1b1b;
  --text: #ddd;
  --muted: #999;
  --border: #2d2d2d;

  --p1: #8fd59a;
  --p2: #3fbf64;
  --p3: #0f8a3f;
  --n1: #e7a0a0;
  --n2: #db5757;
  --n3: #b42222;
}

body {
  margin: 0;
  background: var(--bg);
  color: var(--text);
  font-family: -apple-system, system-ui, Segoe UI, Roboto, Helvetica, Arial, sans-serif;
}

header {
  padding: 10px 14px;
  border-bottom: 1px solid var(--border);
  background: #0f0f0f;
  position: sticky;
  top: 0;
}

h1 {
  font-size: 14px;
  margin: 0;
  font-weight: 600;
}

.grid {
  display: grid;
  gap: 8px;
  padding: 10px;
}

.rowheader {
  grid-column: 1 / -1;
  background: #0f0f0f;
  border: 1px solid var(--border);
  border-radius: 6px;
  padding: 6px 8px;
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 10px;
}

.rowheader .rowtitle {
  font-size: 11px;
  color: var(--muted);
  font-weight: 600;
}

.rowheader .rownote {
  font-size: 11px;
  color: var(--muted);
}

.cell {
  background: var(--panel);
  border: 1px solid var(--border);
  padding: 6px;
  border-radius: 6px;
}

.cell .label {
  font-size: 11px;
  color: var(--muted);
  margin-bottom: 6px;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.micro {
  display: grid;
  grid-auto-rows: 10px;
  gap: 2px;
}

.micro-row {
  display: flex;
  align-items: center;
  gap: 2px;
}

.rlabel {
  width: 34px;
  font-size: 9px;
  color: var(--muted);
  line-height: 10px;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.dot {
  width: 10px;
  height: 10px;
  border-radius: 2px;
  background: transparent;
}

.dot.z0 { background: transparent; }
.dot.p1 { background: var(--p1); }
.dot.p2 { background: var(--p2); }
.dot.p3 { background: var(--p3); }
.dot.n1 { background: var(--n1); }
.dot.n2 { background: var(--n2); }
.dot.n3 { background: var(--n3); }

.legend {
  margin-top: 6px;
  font-size: 11px;
  color: var(--muted);
}

.intent-key {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  margin-top: 6px;
}

.intent-key.inline {
  margin-top: 0;
  gap: 8px;
}

.intent-key .item {
  display: inline-flex;
  align-items: center;
  gap: 6px;
}

.intent-key .col {
  width: 10px;
  height: 10px;
  border-radius: 2px;
  background: transparent;
  border: 1px solid var(--border);
}

.intent-key code {
  font-size: 11px;
  color: var(--text);
}
"""

    if use_grid:
        cols = int(max_x) + 1
        rows = int(max_y) + 1
        grid_style = f"grid-template-columns: repeat({cols}, minmax(160px, 1fr));"
    else:
        cols = 0
        rows = 0
        grid_style = "grid-template-columns: repeat(auto-fill, minmax(160px, 1fr));"

    def cell_html(b: dict[str, Any]) -> str:
        idx = b["idx"]
        filename = b["filename"]
        intents = b["intents"]
        impacts = b["impacts"]
        desc = b.get("desc") or ""

        title_line = f"{idx:02d} — {filename}"
        tooltip = title_line
        if desc:
            tooltip += "\n" + desc
        tooltip += "\n\nRows: " + ", ".join(ROW_KEYS)
        tooltip += "\nIntents: " + ", ".join(intents)

        micro_rows: list[str] = []
        for row in ROW_KEYS:
            dots = []
            for intent_name in intents:
                v = impacts[row].get(intent_name, 0)
                dots.append(
                    f'<div class="dot {impact_class(v)}" title="{html.escape(row)} / {html.escape(intent_name)} = {v}"></div>'
                )
            micro_rows.append(
                '<div class="micro-row">'
                + f'<div class="rlabel" title="{html.escape(row)}">{html.escape(abbreviate_row(row))}</div>'
                + "".join(dots)
                + "</div>"
            )

        return (
            f'<div class="cell" title="{html.escape(tooltip)}">'
            f'<div class="label">{html.escape(title_line)}</div>'
            f'<div class="micro">' + "".join(micro_rows) + "</div>"
            f"</div>"
        )

    # Place blocks into grid and insert per-row intent keys.
    body_cells: list[str] = []

    if use_grid:
        placed = {
            (b.get("x"), b.get("y")): b
            for b in blocks
            if b.get("x") is not None and b.get("y") is not None
        }
        prev_row_intents: tuple[str, ...] | None = None

        for y in range(rows):
            row_blocks = [placed.get((x, y)) for x in range(cols)]
            row_orders = [
                tuple(b["intents"]) for b in row_blocks if b and b.get("intents")
            ]

            row_intents: tuple[str, ...] | None
            row_note = ""
            if not row_orders:
                row_intents = None
            else:
                unique = sorted(set(row_orders), key=lambda t: (len(t), t))
                row_intents = row_orders[0]
                if len(unique) > 1:
                    row_note = f"mixed intents ({len(unique)} sets)"

            if row_intents is not None and row_intents != prev_row_intents:
                body_cells.append(
                    '<div class="rowheader">'
                    + f'<div class="rowtitle">Row {y}</div>'
                    + (
                        f'<div class="rownote">{html.escape(row_note)}</div>'
                        if row_note
                        else ""
                    )
                    + render_intent_key(list(row_intents))
                    + "</div>"
                )
                prev_row_intents = row_intents

            for x in range(cols):
                b = placed.get((x, y))
                if b is None:
                    body_cells.append(
                        '<div class="cell"><div class="label">(empty)</div></div>'
                    )
                else:
                    body_cells.append(cell_html(b))
    else:
        prev: tuple[str, ...] | None = None
        for b in blocks:
            order = tuple(b.get("intents", []))
            if order and order != prev:
                body_cells.append(
                    '<div class="rowheader">'
                    + '<div class="rowtitle">Columns</div>'
                    + render_intent_key(list(order))
                    + "</div>"
                )
                prev = order
            body_cells.append(cell_html(b))

    # Global key just uses the first intent order encountered.
    global_key = ""
    if blocks:
        global_key = render_intent_key(blocks[0]["intents"]).replace("inline", "")

    html_out = f"""<!doctype html>
<html>
<head>
<meta charset=\"utf-8\" />
<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />
<title>{html.escape(title)}</title>
<style>
{css}
.grid {{ {grid_style} }}
</style>
</head>
<body>
<header>
  <h1>{html.escape(title)}</h1>
  <div class=\"legend\">Colors: red = reduction, green = increase, empty = neutral (0).</div>
  <div class=\"legend\">Rows: Motion / Particles / Marks / Geometry. Columns repeated above grid rows when they change.</div>
</header>
<div class=\"grid\">
{"".join(body_cells)}
</div>
</body>
</html>
"""

    out_path.write_text(html_out, encoding="utf-8")


def write_md(out_path: Path, blocks: list[dict[str, Any]]) -> None:
    lines: list[str] = []
    lines.append(f"# Intent Impact Tables (auto-first-pass)\n")
    lines.append("Baseline assumptions:")
    lines.append(
        "- Baseline = **No intent** (Intent Strength = 0, or all activations = 0)"
    )
    lines.append(
        "- Baseline values are config settings with fallback to implementation defaults"
    )
    lines.append("")
    lines.append("Rows: `Motion`, `Particles`, `Marks`, `Geometry`")
    lines.append("")

    for b in blocks:
        idx = b["idx"]
        filename = b["filename"]
        desc = b.get("desc") or ""
        intents = b["intents"]
        impacts = b["impacts"]

        lines.append(f"## {idx:02d} — `{filename}`")
        if desc:
            lines.append(f"> {desc}")
        lines.append("")

        header = "| Mark \\ Intent | " + " | ".join(intents) + " |"
        sep = "|---" + "|---" * len(intents) + "|"
        lines.append(header)
        lines.append(sep)

        for row in ROW_KEYS:
            cells = [str(impacts[row].get(i, 0)) for i in intents]
            lines.append("| " + row + " | " + " | ".join(cells) + " |")

        lines.append("")

    out_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "--configs-dir",
        required=True,
        help="Directory containing synth config JSON files",
    )
    ap.add_argument(
        "--write-ui",
        action="store_true",
        help="Write intents[].ui.impact into config files",
    )
    ap.add_argument(
        "--strip-ui",
        action="store_true",
        help="Strip intents[].ui from config files",
    )
    ap.add_argument(
        "--dry-run",
        action="store_true",
        help="With --strip-ui: report files, do not write",
    )
    ap.add_argument("--out-html", default=None, help="Write compact single-file HTML")
    ap.add_argument("--out-md", default=None, help="Write Markdown tables (optional)")
    ap.add_argument("--title", default="Intent Impact Grid")

    args = ap.parse_args()

    configs_dir = Path(args.configs_dir)
    if not configs_dir.is_dir():
        raise SystemExit(f"configs dir not found: {configs_dir}")

    # Derive openFrameworks root from this script location.
    # .../addons/ofxMarkSynth/scripts/intent_impact.py -> parents[3] == ofRoot
    of_root = Path(__file__).resolve().parents[3]

    if args.strip_ui and args.write_ui:
        raise SystemExit("Use only one of --write-ui or --strip-ui")

    specs_by_type = build_param_specs(of_root)

    config_paths = [Path(p) for p in sorted(glob.glob(str(configs_dir / "*.json")))]

    if args.strip_ui:
        changed_paths: list[Path] = []
        for cfg_path in config_paths:
            data = json.loads(cfg_path.read_text(encoding="utf-8"))
            if strip_ui_from_config(data):
                changed_paths.append(cfg_path)
                if not args.dry_run:
                    cfg_path.write_text(
                        json.dumps(data, indent=2, ensure_ascii=False) + "\n",
                        encoding="utf-8",
                    )

        for p in changed_paths:
            print(p)
        suffix = "would change" if args.dry_run else "changed"
        print(f"{len(changed_paths)} config(s) {suffix}.")
        return

    blocks: list[dict[str, Any]] = []

    for cfg_path in config_paths:
        data = json.loads(cfg_path.read_text(encoding="utf-8"))

        intents = read_intents(data)
        if not intents:
            continue

        bg = data.get("buttonGrid")
        x = y = None
        if isinstance(bg, dict):
            if isinstance(bg.get("x"), (int, float)):
                x = int(bg["x"])
            if isinstance(bg.get("y"), (int, float)):
                y = int(bg["y"])

        m = re.match(r"^(\d+)-", cfg_path.name)
        idx = int(m.group(1)) if m else 0

        impacts = score_config(data, intents, specs_by_type)

        # Ensure all row/intent keys exist.
        for row in ROW_KEYS:
            impacts.setdefault(row, {})
            for it in intents:
                impacts[row].setdefault(it.name, 0)

        if args.write_ui:
            if ensure_ui_written(data, impacts):
                cfg_path.write_text(
                    json.dumps(data, indent=2, ensure_ascii=False) + "\n",
                    encoding="utf-8",
                )

        blocks.append(
            {
                "idx": idx,
                "filename": cfg_path.name,
                "desc": data.get("description")
                if isinstance(data.get("description"), str)
                else "",
                "x": x,
                "y": y,
                "intents": [i.name for i in intents],
                "impacts": impacts,
            }
        )

    if args.out_md:
        write_md(Path(args.out_md), blocks)

    if args.out_html:
        write_html(Path(args.out_html), args.title, blocks)


if __name__ == "__main__":
    main()
