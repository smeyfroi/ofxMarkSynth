#!/usr/bin/env python3
"""Validate MarkSynth synth config JSON connections.

This tool checks:
- JSON parses
- All connection endpoints are valid for the referenced Mod types

It is designed to run in a stable Dockerized Python runtime.

How it stays in sync with MarkSynth:
- It parses the ofxMarkSynth C++ sources and extracts the literal keys used in
  `sourceNameIdMap` and `sinkNameIdMap`.
- It also parses `ofParameter<...> foo { "Name", ... }` declarations and resolves
  entries like `{ foo.getName(), SOME_ID }`.

Exit codes:
- 0: all configs valid
- 2: at least one config invalid
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple


@dataclass(frozen=True)
class ModSpec:
    type_name: str
    sources: Set[str]
    sinks: Set[str]
    source_files: List[str]


class ValidationError(Exception):
    pass


PARAM_RE = re.compile(
    r"ofParameter\s*<[^>]+>\s+(\w+)\s*\{\s*\"([^\"]+)\"", re.MULTILINE
)
MAP_RE = re.compile(r"(sourceNameIdMap|sinkNameIdMap)\s*=\s*\{(.*?)\};", re.DOTALL)
# Special-case extraction for Synth memory bank sinks, which are returned from a helper.
MEMORYBANK_SINK_RETURN_RE = re.compile(
    r"MemoryBankController::getSinkNameIdMap\s*\([^)]*\)\s*const\s*\{.*?return\s*\{(.*?)\}\s*;",
    re.DOTALL,
)
ENTRY_STRING_RE = re.compile(r"\{\s*\"([^\"]+)\"\s*,")
ENTRY_GETNAME_RE = re.compile(r"\{\s*(\w+)\s*\.\s*getName\s*\(\s*\)\s*,")
# Also support entries added outside initializer lists, e.g. `sinkNameIdMap["Trigger"] = ...`.
ENTRY_BRACKET_ASSIGN_RE = re.compile(
    r"(sourceNameIdMap|sinkNameIdMap)\s*\[\s*\"([^\"]+)\"\s*\]"
)

# Semantic validation: which Mods can actually seed a layer with non-empty pixels.
# Used to detect drawn Fluid value layers that never receive marks.
SEED_MARK_MOD_TYPES: Set[str] = {
    "SoftCircle",
    "SandLine",
    "ParticleSet",
    "ParticleField",
    "DividedArea",
    "Collage",
    "Path",
    "Text",
}


def iter_config_paths(target: Path) -> List[Path]:
    if target.is_file():
        return [target]
    if target.is_dir():
        return sorted([p for p in target.glob("*.json") if p.is_file()])
    raise ValidationError(f"Path not found: {target}")


def load_json(path: Path) -> dict:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:
        raise ValidationError(f"JSON parse failed: {path}: {exc}")


def _read_text_if_exists(path: Path) -> str:
    if path.exists() and path.is_file():
        return path.read_text(encoding="utf-8", errors="replace")
    return ""


def _extract_param_names(contents: str) -> Dict[str, str]:
    """Return mapping of parameter variable name -> parameter display name."""
    return {m.group(1): m.group(2) for m in PARAM_RE.finditer(contents)}


def _extract_map_keys(
    contents: str, map_name: str, param_names: Dict[str, str]
) -> Set[str]:
    keys: Set[str] = set()

    # Initializer-list style: `sinkNameIdMap = { { "Foo", ... }, { bar.getName(), ... } };`
    for m in MAP_RE.finditer(contents):
        if m.group(1) != map_name:
            continue
        block = m.group(2)
        for s in ENTRY_STRING_RE.finditer(block):
            keys.add(s.group(1))
        for g in ENTRY_GETNAME_RE.finditer(block):
            var = g.group(1)
            if var in param_names:
                keys.add(param_names[var])

    # Bracket-assignment style: `sinkNameIdMap["Trigger"] = ...;`
    # (PathMod uses this to conditionally expose Trigger.)
    for s in ENTRY_BRACKET_ASSIGN_RE.finditer(contents):
        if s.group(1) == map_name:
            keys.add(s.group(2))

    return keys


def _find_mod_files(src_root: Path, mod_type: str) -> List[Path]:
    """Find C++ files likely defining a given mod type."""
    # Typical convention: <Type>Mod.cpp/.hpp
    cpp_name = f"{mod_type}Mod.cpp"
    hpp_name = f"{mod_type}Mod.hpp"

    candidates: List[Path] = []
    for p in src_root.rglob(cpp_name):
        candidates.append(p)
    for p in src_root.rglob(hpp_name):
        candidates.append(p)

    # Special case: Synth sink/source mappings live in Synth.cpp and are extended
    # at runtime with MemoryBankController sinks.
    if mod_type == "Synth":
        synth_cpp = src_root / "core" / "Synth.cpp"
        synth_hpp = src_root / "core" / "Synth.hpp"
        mem_cpp = src_root / "controller" / "MemoryBankController.cpp"
        mem_hpp = src_root / "controller" / "MemoryBankController.hpp"
        candidates = [
            p for p in [synth_cpp, synth_hpp, mem_cpp, mem_hpp] if p.exists()
        ] + candidates

    # Deduplicate
    dedup: List[Path] = []
    seen = set()
    for p in candidates:
        if str(p) in seen:
            continue
        seen.add(str(p))
        dedup.append(p)
    return dedup


def _extract_memorybank_sink_keys(
    contents: str, param_names: Dict[str, str]
) -> Set[str]:
    keys: Set[str] = set()
    m = MEMORYBANK_SINK_RETURN_RE.search(contents)
    if not m:
        return keys
    block = m.group(1)
    for s in ENTRY_STRING_RE.finditer(block):
        keys.add(s.group(1))
    for g in ENTRY_GETNAME_RE.finditer(block):
        var = g.group(1)
        if var in param_names:
            keys.add(param_names[var])
    return keys


def build_specs(
    ofxmarksynth_root: Path, mod_types: Set[str], strict: bool
) -> Dict[str, ModSpec]:
    src_root = ofxmarksynth_root / "src"
    if not src_root.exists():
        raise ValidationError(
            f"Invalid ofxMarkSynth root (no src/): {ofxmarksynth_root}"
        )

    specs: Dict[str, ModSpec] = {}

    for mod_type in sorted(mod_types):
        files = _find_mod_files(src_root, mod_type)
        if not files:
            if strict:
                raise ValidationError(
                    f"No C++ files found for mod type '{mod_type}' under: {src_root}"
                )
            continue

        # Parse parameter names across all candidate files
        param_names: Dict[str, str] = {}
        all_contents: List[Tuple[str, str]] = []
        for f in files:
            text = _read_text_if_exists(f)
            all_contents.append((str(f), text))
            param_names.update(_extract_param_names(text))

        sources: Set[str] = set()
        sinks: Set[str] = set()
        for _, text in all_contents:
            sources |= _extract_map_keys(text, "sourceNameIdMap", param_names)
            sinks |= _extract_map_keys(text, "sinkNameIdMap", param_names)

        # Synth: extend sink list with MemoryBankController sinks.
        if mod_type == "Synth":
            for _, text in all_contents:
                sinks |= _extract_memorybank_sink_keys(text, param_names)

        # If a file exists but we couldn't find any map assignments, keep empty sets.
        specs[mod_type] = ModSpec(
            type_name=mod_type,
            sources=sources,
            sinks=sinks,
            source_files=[str(f) for f in files],
        )

    return specs


def parse_connection(conn: str) -> Optional[tuple[str, str, str, str]]:
    """Parse 'A.Source -> B.Sink Name' or '.Source -> B.Sink' or 'A.Source -> .Sink'."""
    if "->" not in conn:
        return None
    left, right = [s.strip() for s in conn.split("->", 1)]

    if left.startswith("."):
        src_mod = "."
        src_name = left[1:]
    else:
        if "." not in left:
            return None
        src_mod, src_name = left.split(".", 1)

    if right.startswith("."):
        dst_mod = "."
        dst_name = right[1:]
    else:
        if "." not in right:
            return None
        dst_mod, dst_name = right.split(".", 1)

    return src_mod, src_name, dst_mod, dst_name


def validate_connections(
    config: dict, path: Path, specs: Dict[str, ModSpec], strict: bool
) -> List[str]:
    mods: dict = config.get("mods", {})
    if not isinstance(mods, dict):
        return ["mods must be an object"]

    mod_types: Dict[str, str] = {}
    for mod_name, mod_spec in mods.items():
        if not isinstance(mod_spec, dict):
            continue
        mod_types[mod_name] = str(mod_spec.get("type", ""))

    issues: List[str] = []

    conns = config.get("connections", [])
    if not isinstance(conns, list):
        return ["connections must be an array"]

    for i, conn in enumerate(conns):
        if not isinstance(conn, str):
            issues.append(f"connections[{i}] must be a string")
            continue

        parsed = parse_connection(conn)
        if not parsed:
            issues.append(f"Bad connection format: {conn}")
            continue

        src_mod, src_name, dst_mod, dst_name = parsed

        if src_mod != "." and src_mod not in mods:
            issues.append(f"Unknown source mod '{src_mod}' in: {conn}")
            continue
        if dst_mod != "." and dst_mod not in mods:
            issues.append(f"Unknown sink mod '{dst_mod}' in: {conn}")
            continue

        # Source validation
        if src_mod == ".":
            synth_spec = specs.get("Synth")
            if synth_spec and synth_spec.sources and src_name not in synth_spec.sources:
                issues.append(
                    f"Bad source '.{src_name}' in: {conn} (allowed: {sorted(synth_spec.sources)})"
                )
        else:
            src_type = mod_types.get(src_mod, "")
            src_spec = specs.get(src_type)
            if not src_spec:
                if strict and src_type:
                    issues.append(
                        f"No validator spec for mod type '{src_type}' (source side) in: {conn}"
                    )
            else:
                if src_spec.sources and src_name not in src_spec.sources:
                    issues.append(
                        f"Bad source '{src_mod}.{src_name}' in: {conn} (allowed: {sorted(src_spec.sources)})"
                    )

        # Sink validation
        if dst_mod == ".":
            synth_spec = specs.get("Synth")
            if synth_spec and synth_spec.sinks and dst_name not in synth_spec.sinks:
                issues.append(
                    f"Bad sink '.{dst_name}' in: {conn} (allowed: {sorted(synth_spec.sinks)})"
                )
        else:
            dst_type = mod_types.get(dst_mod, "")
            dst_spec = specs.get(dst_type)
            if not dst_spec:
                if strict and dst_type:
                    issues.append(
                        f"No validator spec for mod type '{dst_type}' (sink side) in: {conn}"
                    )
            else:
                if dst_spec.sinks and dst_name not in dst_spec.sinks:
                    issues.append(
                        f"Bad sink '{dst_mod}.{dst_name}' in: {conn} (allowed: {sorted(dst_spec.sinks)})"
                    )

    return issues


def validate_semantics(config: dict) -> tuple[List[str], List[str]]:
    """Config-level semantic checks beyond endpoint name validation.

    Returns:
      (errors, warnings)
    """

    errors: List[str] = []
    warnings: List[str] = []

    layers = config.get("drawingLayers", {})
    mods = config.get("mods", {})
    if not isinstance(layers, dict) or not isinstance(mods, dict):
        return (errors, warnings)

    # Build mod type map
    mod_types: Dict[str, str] = {}
    for mod_name, mod_spec in mods.items():
        if isinstance(mod_spec, dict):
            mod_types[mod_name] = str(mod_spec.get("type", ""))

    def iter_assigned_layers(mod_spec: dict) -> Set[str]:
        layer_map = mod_spec.get("layers", {})
        if not isinstance(layer_map, dict):
            return set()
        out: Set[str] = set()
        for layer_list in layer_map.values():
            if not isinstance(layer_list, list):
                continue
            for layer in layer_list:
                if isinstance(layer, str):
                    out.add(layer)
        return out

    def layer_is_seeded(layer_name: str, *, exclude_mod: Optional[str] = None) -> bool:
        for name, m in mods.items():
            if exclude_mod and name == exclude_mod:
                continue
            if not isinstance(m, dict):
                continue
            if mod_types.get(name, "") not in SEED_MARK_MOD_TYPES:
                continue
            if layer_name in iter_assigned_layers(m):
                return True
        return False

    clearing_layers: Set[str] = set()
    for layer_name, layer_spec in layers.items():
        if isinstance(layer_spec, dict) and layer_spec.get("clearOnUpdate", False):
            clearing_layers.add(layer_name)

    fade_mods_by_layer: Dict[str, List[str]] = {}
    for mod_name, mod_spec in mods.items():
        if not isinstance(mod_spec, dict):
            continue
        if mod_types.get(mod_name, "") != "Fade":
            continue
        for layer_name in iter_assigned_layers(mod_spec):
            fade_mods_by_layer.setdefault(layer_name, []).append(mod_name)

    fluid_values_layers: Set[str] = set()

    # Fluid values layer should either be intentionally unused (isDrawn=false and small)
    # or it should be seeded by a mark-making Mod.
    # Additionally: never apply Fade to a Fluid values layer (use Value Dissipation instead).
    for fluid_name, mod_spec in mods.items():
        if not isinstance(mod_spec, dict):
            continue
        if mod_spec.get("type") != "Fluid":
            continue

        layer_map = mod_spec.get("layers", {})
        if not isinstance(layer_map, dict):
            continue

        for layer_name in iter_assigned_layers(mod_spec):
            if layer_name in clearing_layers:
                errors.append(
                    f"Fluid targets layer '{layer_name}' but that layer has clearOnUpdate=true. "
                    "Layers that clear each frame are not valid targets for Fluid."
                )

        values_layers = layer_map.get("default", [])
        if not isinstance(values_layers, list):
            continue

        for values_layer in values_layers:
            if not isinstance(values_layer, str):
                continue

            values_layer_spec = layers.get(values_layer)
            if not isinstance(values_layer_spec, dict):
                errors.append(
                    f"Fluid values layer '{values_layer}' is not defined in drawingLayers"
                )
                continue

            fluid_values_layers.add(values_layer)

            if values_layer in clearing_layers:
                errors.append(
                    f"Fluid values layer '{values_layer}' has clearOnUpdate=true. "
                    "Remove clearOnUpdate and rely on Fluid 'Value Dissipation' for decay."
                )

            if values_layer in fade_mods_by_layer:
                fade_names = ", ".join(sorted(fade_mods_by_layer[values_layer]))
                errors.append(
                    f"Fluid values layer '{values_layer}' also has Fade mod(s): {fade_names}. "
                    "Remove Fade and use Fluid 'Value Dissipation' for decay."
                )

            is_drawn = values_layer_spec.get("isDrawn", True)

            # If values are drawn, require that at least one mark-making Mod targets that layer.
            if is_drawn:
                seeded = False
                for other_name, other_mod in mods.items():
                    if other_name == fluid_name or not isinstance(other_mod, dict):
                        continue

                    other_type = mod_types.get(other_name, "")
                    if other_type not in SEED_MARK_MOD_TYPES:
                        continue

                    other_layers = other_mod.get("layers", {})
                    if not isinstance(other_layers, dict):
                        continue

                    for layer_list in other_layers.values():
                        if isinstance(layer_list, list) and values_layer in layer_list:
                            seeded = True
                            break
                    if seeded:
                        break

                if not seeded:
                    errors.append(
                        "Fluid values layer '"
                        + values_layer
                        + "' isDrawn=true but no mark-making Mod draws into it. "
                        + "Either: (1) switch to velocity-only carrier mode (set isDrawn=false and consider shrinking the values layer), "
                        + "or (2) add a mark Mod (e.g. SoftCircle) targeting that layer."
                    )

            # If values are not drawn, we assume velocity-only carrier mode.
            # In that mode, it’s usually a mistake if nothing consumes velocitiesTexture.
            else:
                conns = config.get("connections", [])
                used_velocities_texture = False
                if isinstance(conns, list):
                    for c in conns:
                        if not isinstance(c, str):
                            continue
                        parsed = parse_connection(c)
                        if not parsed:
                            continue
                        src_mod, src_name, _dst_mod, _dst_name = parsed
                        if src_mod == fluid_name and src_name == "velocitiesTexture":
                            used_velocities_texture = True
                            break

                if not used_velocities_texture:
                    warnings.append(
                        "Fluid is in velocity-only mode (values layer '"
                        + values_layer
                        + "' isDrawn=false) but "
                        + fluid_name
                        + ".velocitiesTexture is not used by any connection. "
                        + "Either connect it (e.g. to Smear/ParticleField) or remove Fluid/velocities layer."
                    )

    # Fade/Smear are layer processors: their target layer must be seeded with marks.
    # This remains valid for latent-world and fresh-entrance layers.
    # Additional rule: avoid stacking Fade+Smear on the same layer.

    layer_to_processors: Dict[str, Set[str]] = {}
    for proc_name, mod_spec in mods.items():
        if not isinstance(mod_spec, dict):
            continue
        mod_type = str(mod_spec.get("type", ""))
        if mod_type not in {"Fade", "Smear"}:
            continue

        for layer_name in sorted(iter_assigned_layers(mod_spec)):
            if layer_name not in layers:
                errors.append(
                    f"{mod_type} targets layer '{layer_name}' but that layer is not defined in drawingLayers"
                )
                continue

            if layer_name in clearing_layers:
                errors.append(
                    f"{mod_type} targets layer '{layer_name}' but that layer has clearOnUpdate=true. "
                    "Layers that clear each frame are not valid targets for Fade/Smear."
                )
                continue

            layer_to_processors.setdefault(layer_name, set()).add(mod_type)

            if not layer_is_seeded(layer_name, exclude_mod=proc_name):
                errors.append(
                    f"{mod_type} targets layer '{layer_name}' but no mark-making Mod draws into it. "
                    "Add a mark Mod targeting the same layer (e.g. SoftCircle/SandLine/DividedArea), or remove the processor."
                )

    for layer_name, processors in sorted(layer_to_processors.items()):
        if processors == {"Fade", "Smear"}:
            errors.append(
                f"Layer '{layer_name}' has both Fade and Smear. Use only one layer processor per layer (prefer Smear's MixNew/AlphaMultiplier for persistence)."
            )

    # Fluid boundary mode must match layer wrap.
    # The sim validates this strictly (e.g. SolidWalls/Open => clamp, Wrap => repeat).
    expected_wrap_by_boundary_mode: Dict[int, str] = {
        0: "GL_CLAMP_TO_EDGE",  # SolidWalls
        1: "GL_REPEAT",  # Wrap
        2: "GL_CLAMP_TO_EDGE",  # Open
    }

    for fluid_name, fluid_spec in _iter_mods_of_type(mods, "Fluid"):
        fluid_cfg = fluid_spec.get("config", {})
        if not isinstance(fluid_cfg, dict):
            fluid_cfg = {}

        boundary_mode_raw = fluid_cfg.get("Boundary Mode")
        boundary_mode_f = _safe_float(boundary_mode_raw)
        boundary_mode = int(boundary_mode_f) if boundary_mode_f is not None else 0
        expected_wrap = expected_wrap_by_boundary_mode.get(
            boundary_mode, "GL_CLAMP_TO_EDGE"
        )

        layer_map = fluid_spec.get("layers", {})
        if not isinstance(layer_map, dict):
            continue

        values_layers = layer_map.get("default", [])
        vel_layers = layer_map.get("velocities", [])
        if not isinstance(values_layers, list) or not isinstance(vel_layers, list):
            continue

        def check_layer_wrap(layer_name: str) -> None:
            spec = layers.get(layer_name)
            if not isinstance(spec, dict):
                return
            wrap = spec.get("wrap")
            if wrap != expected_wrap:
                errors.append(
                    f"Fluid '{fluid_name}' boundary mode {boundary_mode} expects wrap={expected_wrap} but layer '{layer_name}' has wrap={wrap}"
                )

        for layer_name in values_layers:
            if isinstance(layer_name, str):
                check_layer_wrap(layer_name)

        for layer_name in vel_layers:
            if isinstance(layer_name, str):
                check_layer_wrap(layer_name)

    # dt consistency: if a FluidRadialImpulse draws into a Fluid's velocities layer, their dt must match.
    # (We intentionally do not auto-sync this in code; enforce it in configs.)

    # Map: layerName -> list of Fluid mods that bind it as a velocities layer
    fluid_vel_layers: Dict[str, List[str]] = {}
    fluid_dt_by_name: Dict[str, Optional[float]] = {}

    for fluid_name, fluid_spec in _iter_mods_of_type(mods, "Fluid"):
        fluid_cfg = fluid_spec.get("config", {})
        if not isinstance(fluid_cfg, dict):
            fluid_cfg = {}
        fluid_dt_by_name[fluid_name] = _safe_float(fluid_cfg.get("dt"))

        layer_map = fluid_spec.get("layers", {})
        if not isinstance(layer_map, dict):
            continue

        vel_layers = layer_map.get("velocities", [])
        if not isinstance(vel_layers, list):
            continue

        for layer in vel_layers:
            if isinstance(layer, str):
                fluid_vel_layers.setdefault(layer, []).append(fluid_name)

    # Validate impulses that target any velocities layer.
    for impulse_name, impulse_spec in _iter_mods_of_type(mods, "FluidRadialImpulse"):
        impulse_cfg = impulse_spec.get("config", {})
        if not isinstance(impulse_cfg, dict):
            impulse_cfg = {}

        impulse_dt = _safe_float(impulse_cfg.get("dt"))

        layer_map = impulse_spec.get("layers", {})
        if not isinstance(layer_map, dict):
            continue

        target_layers = layer_map.get("default", [])
        if not isinstance(target_layers, list):
            continue

        for layer in target_layers:
            if not isinstance(layer, str):
                continue

            for fluid_name in fluid_vel_layers.get(layer, []):
                fluid_dt = fluid_dt_by_name.get(fluid_name)

                if fluid_dt is None:
                    errors.append(
                        f"Fluid '{fluid_name}' is missing/invalid config dt; required for dt consistency checks"
                    )
                    continue

                if impulse_dt is None:
                    errors.append(
                        f"FluidRadialImpulse '{impulse_name}' targets velocities layer '{layer}' but is missing/invalid config dt"
                    )
                    continue

                if abs(fluid_dt - impulse_dt) > 1.0e-6:
                    errors.append(
                        f"dt mismatch: Fluid '{fluid_name}' dt={fluid_dt} but FluidRadialImpulse '{impulse_name}' dt={impulse_dt} (shared velocities layer '{layer}')"
                    )

    # ParticleField must draw into a layer that decays.
    # This prevents fast accumulation and blown-out registers.
    decay_layers = (
        set(layer_to_processors.keys()) | fluid_values_layers | clearing_layers
    )
    for mod_name, mod_spec in mods.items():
        if not isinstance(mod_spec, dict):
            continue
        if mod_types.get(mod_name, "") != "ParticleField":
            continue

        for layer_name in sorted(iter_assigned_layers(mod_spec)):
            if layer_name not in layers:
                errors.append(
                    f"ParticleField '{mod_name}' targets layer '{layer_name}' but that layer is not defined in drawingLayers"
                )
                continue

            if layer_name not in decay_layers:
                errors.append(
                    f"ParticleField '{mod_name}' targets layer '{layer_name}' but that layer has no decay. "
                    "ParticleField layers must also be targeted by a Fade, Smear, Fluid (values) Mod, or have clearOnUpdate=true."
                )

    # Collage Colour sink semantics:
    # - Strategy 2 draws an untinted snapshot, so Colour sink is ignored.
    # - Strategies 0/1 use the Colour parameter (either via connection or manual config).
    conns = config.get("connections", [])
    collage_colour_connected: Set[str] = set()
    if isinstance(conns, list):
        for c in conns:
            if not isinstance(c, str):
                continue
            parsed = parse_connection(c)
            if not parsed:
                continue
            _src_mod, _src_name, dst_mod, dst_name = parsed
            if dst_name != "Colour":
                continue
            if mod_types.get(dst_mod, "") == "Collage":
                collage_colour_connected.add(dst_mod)

    for mod_name, mod_spec in mods.items():
        if not isinstance(mod_spec, dict):
            continue
        if mod_types.get(mod_name, "") != "Collage":
            continue

        mod_cfg = mod_spec.get("config", {})
        if not isinstance(mod_cfg, dict):
            mod_cfg = {}

        strategy_raw = mod_cfg.get("Strategy")
        try:
            strategy = int(strategy_raw) if strategy_raw is not None else 1
        except Exception:
            strategy = 1

        has_manual_colour = "Colour" in mod_cfg
        has_colour_connection = mod_name in collage_colour_connected

        if strategy == 2 and has_colour_connection:
            warnings.append(
                f"Collage '{mod_name}' has Strategy=2 (untinted snapshot) but receives '{mod_name}.Colour' via connections. "
                "This sink is ignored for Strategy=2."
            )

        if strategy != 2 and (not has_colour_connection) and (not has_manual_colour):
            warnings.append(
                f"Collage '{mod_name}' uses Strategy={strategy} but has no 'Colour' in its config and no connection to '{mod_name}.Colour'. "
                "It will default to white."
            )

    return (errors, warnings)


def _safe_float(value: object) -> Optional[float]:
    if value is None:
        return None
    if isinstance(value, (int, float)):
        return float(value)
    if isinstance(value, str):
        try:
            return float(value.strip())
        except Exception:
            return None
    return None


def _infer_rc_from_filename(path: Path) -> Optional[tuple[int, int]]:
    m = re.match(r"^(\d)(\d)-", path.name)
    if not m:
        return None
    return (int(m.group(1)), int(m.group(2)))


def _iter_mods_of_type(mods: dict, type_name: str) -> List[tuple[str, dict]]:
    out: List[tuple[str, dict]] = []
    if not isinstance(mods, dict):
        return out
    for name, spec in mods.items():
        if not isinstance(spec, dict):
            continue
        if str(spec.get("type", "")) == type_name:
            out.append((name, spec))
    return out


def _mod_types_map(mods: dict) -> Dict[str, str]:
    mod_types: Dict[str, str] = {}
    if not isinstance(mods, dict):
        return mod_types
    for mod_name, mod_spec in mods.items():
        if isinstance(mod_spec, dict):
            mod_types[mod_name] = str(mod_spec.get("type", ""))
    return mod_types


def validate_policy_improvisation1(
    config: dict, path: Path
) -> tuple[List[str], List[str]]:
    """Improvisation1-specific checks based on the design notes.

    Kept opt-in via --policy improvisation1 to avoid constraining other config sets.
    """

    errors: List[str] = []
    warnings: List[str] = []

    button_grid = config.get("buttonGrid")
    if not isinstance(button_grid, dict):
        errors.append("Missing/invalid buttonGrid (required for Improvisation1 policy)")
        return (errors, warnings)

    x = button_grid.get("x")
    y = button_grid.get("y")
    color = button_grid.get("color")

    if not isinstance(x, int) or not isinstance(y, int):
        errors.append("buttonGrid.x and buttonGrid.y must be integers")
        return (errors, warnings)

    if x < 0 or x > 7 or y < 0 or y > 6:
        errors.append(f"buttonGrid out of range: x={x}, y={y} (expected 0–7, 0–6)")

    # Filename prefix must map to controller grid slot.
    rc = _infer_rc_from_filename(path)
    if rc is None:
        errors.append(
            f"Filename must start with RC- prefix (e.g. '00-...'): got '{path.name}'"
        )
    else:
        row_from_name, col_from_name = rc
        if row_from_name != y or col_from_name != x:
            errors.append(
                f"Filename RC prefix {row_from_name}{col_from_name} disagrees with buttonGrid x/y {x},{y}"
            )

    # Controller pad color scheme (from design notes).
    expected_colors_by_row: Dict[int, List[str]] = {
        0: [
            "#FF0000",
            "#FF5500",
            "#FFAA00",
            "#FFFF00",
            "#00FF00",
            "#00FFAA",
            "#0055FF",
            "#8000FF",
        ],
        1: [
            "#FF1919",
            "#FF6619",
            "#FFB219",
            "#FFFF19",
            "#19FF19",
            "#19FFB2",
            "#1966FF",
            "#8C19FF",
        ],
        2: [
            "#FF3333",
            "#FF7733",
            "#FFBB33",
            "#FFFF33",
            "#33FF33",
            "#33FFBB",
            "#3377FF",
            "#9933FF",
        ],
        3: [
            "#FF4D4D",
            "#FF884D",
            "#FFC34D",
            "#FFFF4D",
            "#4DFF4D",
            "#4DFFC3",
            "#4D88FF",
            "#A64DFF",
        ],
        4: [
            "#FF6666",
            "#FF9966",
            "#FFCC66",
            "#FFFF66",
            "#66FF66",
            "#66FFCC",
            "#6699FF",
            "#B266FF",
        ],
        5: [
            "#FF8080",
            "#FFAA80",
            "#FFD480",
            "#FFFF80",
            "#80FF80",
            "#80FFD4",
            "#80AAFF",
            "#BF80FF",
        ],
        6: [
            "#FF9999",
            "#FFBB99",
            "#FFDD99",
            "#FFFF99",
            "#99FF99",
            "#99FFDD",
            "#99BBFF",
            "#CC99FF",
        ],
    }

    if not isinstance(color, str):
        errors.append("buttonGrid.color must be a hex string")
    else:
        expected = expected_colors_by_row.get(y, [None] * 8)[x]
        if expected and color.upper() != expected:
            errors.append(
                f"buttonGrid.color {color} does not match expected {expected} for x={x}, y={y}"
            )

    # Background color wiring policy: use palette darkest as an agency lane.
    # Enforce that BackgroundColour is connected from a SomPalette.Darkest source.
    conns = config.get("connections", [])
    if not isinstance(conns, list):
        conns = []

    mods = config.get("mods", {})
    mod_types = _mod_types_map(mods)

    palette_mod_names = [
        name
        for name, t in mod_types.items()
        if t == "SomPalette" and isinstance(name, str)
    ]

    bg_conns = [
        c
        for c in conns
        if isinstance(c, str) and c.strip().endswith("-> .BackgroundColour")
    ]
    if not bg_conns:
        errors.append(
            "Missing connection to '.BackgroundColour' (expected palette-derived background)"
        )
    else:
        # Allow exactly one background connection; multiple likely indicates accidental layering.
        if len(bg_conns) > 1:
            warnings.append(
                f"Multiple BackgroundColour connections found ({len(bg_conns)}); expected 1"
            )

        ok = False
        for c in bg_conns:
            parsed = parse_connection(c)
            if not parsed:
                continue
            src_mod, src_name, dst_mod, dst_name = parsed
            if dst_mod == "." and dst_name == "BackgroundColour":
                if (
                    src_mod != "."
                    and mod_types.get(src_mod) == "SomPalette"
                    and src_name == "Darkest"
                ):
                    ok = True
                    break
        if not ok:
            if palette_mod_names:
                errors.append(
                    "BackgroundColour must be driven by SomPalette.Darkest (e.g. 'Palette.Darkest -> .BackgroundColour')"
                )
            else:
                errors.append(
                    "BackgroundColour connection present but no SomPalette mod exists (Improvisation1 requires SomPalette)"
                )

    # Visibility floors (Improvisation1 policy): row-specific minima for substrate layers.
    # Only enforce if the layer is drawn and defines alpha.
    layer_alpha_mins_by_row: Dict[int, Dict[str, float]] = {
        0: {"ground": 0.35, "wash": 0.35},
        1: {"ground": 0.30, "wash": 0.30},
        2: {"ground": 0.35, "wash": 0.35},
        3: {"ground": 0.35, "wash": 0.35},
        4: {"ground": 0.30, "wash": 0.30},
        5: {"ground": 0.50, "wash": 0.50},
        6: {"ground": 0.38, "wash": 0.38},
    }

    layers = config.get("drawingLayers", {})
    if isinstance(layers, dict):
        for layer_name, min_alpha in layer_alpha_mins_by_row.get(y, {}).items():
            layer_spec = layers.get(layer_name)
            if not isinstance(layer_spec, dict):
                continue
            if not layer_spec.get("isDrawn", True):
                continue
            alpha_val = _safe_float(layer_spec.get("alpha"))
            if alpha_val is None:
                warnings.append(
                    f"drawingLayers.{layer_name} isDrawn=true but has no numeric alpha; expected >= {min_alpha}"
                )
            elif alpha_val < min_alpha:
                errors.append(
                    f"drawingLayers.{layer_name}.alpha={alpha_val} below row {y} minimum {min_alpha}"
                )

            if layer_spec.get("paused") is True:
                warnings.append(
                    f"drawingLayers.{layer_name} is paused=true; Improvisation1 prefers live, low-alpha layers"
                )

    # Bed-ish SoftCircle visibility floors: any SoftCircle drawing into ground/wash.
    bed_softcircle_alpha_mins_by_row: Dict[int, float] = {
        0: 0.12,
        1: 0.12,
        2: 0.16,
        3: 0.14,
        4: 0.12,
        5: 0.12,
        6: 0.14,
    }
    bed_min = bed_softcircle_alpha_mins_by_row.get(y)

    def _softcircle_targets_bed_layer(mod_spec: dict) -> bool:
        layer_map = mod_spec.get("layers", {})
        if not isinstance(layer_map, dict):
            return False
        for layer_list in layer_map.values():
            if not isinstance(layer_list, list):
                continue
            for layer in layer_list:
                if layer in {"ground", "wash"}:
                    return True
        return False

    for mod_name, mod_spec in _iter_mods_of_type(mods, "SoftCircle"):
        if not _softcircle_targets_bed_layer(mod_spec):
            continue

        mod_cfg = mod_spec.get("config", {})
        if not isinstance(mod_cfg, dict):
            mod_cfg = {}

        alpha_mult = _safe_float(mod_cfg.get("AlphaMultiplier"))
        if alpha_mult is None:
            warnings.append(
                f"SoftCircle '{mod_name}' targets ground/wash but has no numeric config.AlphaMultiplier; expected >= {bed_min}"
            )
        elif bed_min is not None and alpha_mult < bed_min:
            errors.append(
                f"SoftCircle '{mod_name}' targets ground/wash with AlphaMultiplier={alpha_mult} below row {y} minimum {bed_min}"
            )

    # ParticleSet performance rule: explicitly specify the three expensive defaults.
    for mod_name, mod_spec in _iter_mods_of_type(mods, "ParticleSet"):
        mod_cfg = mod_spec.get("config", {})
        if not isinstance(mod_cfg, dict):
            mod_cfg = {}
        required = ["maxParticles", "maxParticleAge", "connectionRadius"]
        missing = [k for k in required if k not in mod_cfg]
        if missing:
            errors.append(
                f"ParticleSet '{mod_name}' missing config keys: {', '.join(missing)} (must override ofxParticleSet defaults)"
            )

    # Fade alpha mapping rule: never wire raw Audio.RmsScalar directly into Fade.Alpha.
    fade_mod_names = {name for name, t in mod_types.items() if t == "Fade"}
    for c in conns:
        if not isinstance(c, str):
            continue
        parsed = parse_connection(c)
        if not parsed:
            continue
        src_mod, src_name, dst_mod, dst_name = parsed
        if (
            src_mod == "Audio"
            and src_name == "RmsScalar"
            and dst_mod in fade_mod_names
            and dst_name == "Alpha"
        ):
            errors.append(
                f"Direct 'Audio.RmsScalar -> {dst_mod}.Alpha' is forbidden; map via MultiplyAdd to a safe fade range"
            )

    # Description tag policy: include [audPts: ...] tag for quick reference.
    desc = config.get("description")
    if isinstance(desc, str):
        if "[audPts:" not in desc:
            warnings.append("description missing '[audPts: ...]' tag")
    else:
        warnings.append("description missing or not a string")

    return (errors, warnings)


def main(argv: Optional[List[str]] = None) -> int:
    parser = argparse.ArgumentParser(
        description="Validate MarkSynth synth config JSON files"
    )
    parser.add_argument("target", help="Path to a .json file or directory of configs")
    parser.add_argument(
        "--ofxmarksynth-root",
        default=None,
        help="Path to ofxMarkSynth addon root (must contain src/)",
    )
    parser.add_argument(
        "--strict",
        action="store_true",
        help="Fail if validator cannot find a spec for a referenced mod type",
    )
    parser.add_argument(
        "--policy",
        default="none",
        choices=["none", "improvisation1"],
        help=(
            "Enable additional opinionated checks. 'improvisation1' enforces the "
            "Improvisation1 performance-set policies documented in the design notes."
        ),
    )

    args = parser.parse_args(argv)
    target = Path(args.target)

    try:
        config_paths = iter_config_paths(target)
    except ValidationError as exc:
        print(str(exc), file=sys.stderr)
        return 2

    if not config_paths:
        print(f"No .json files found in: {target}", file=sys.stderr)
        return 2

    ofx_root_str = args.ofxmarksynth_root or os.environ.get("OFXMARKSYNTH_ROOT")
    if not ofx_root_str:
        print(
            "Missing --ofxmarksynth-root (or env OFXMARKSYNTH_ROOT). "
            "This is required so we can parse sourceNameIdMap/sinkNameIdMap from C++.",
            file=sys.stderr,
        )
        return 2

    try:
        ofx_root = Path(ofx_root_str)
        # determine mod types referenced
        referenced_types: Set[str] = {"Synth"}
        for p in config_paths:
            cfg = load_json(p)
            mods = cfg.get("mods", {})
            if isinstance(mods, dict):
                for m in mods.values():
                    if isinstance(m, dict) and m.get("type"):
                        referenced_types.add(str(m.get("type")))

        specs = build_specs(ofx_root, referenced_types, args.strict)

        failed = 0
        warnings_total = 0
        for p in config_paths:
            cfg = load_json(p)
            issues = validate_connections(cfg, p, specs, args.strict)
            semantic_errors, semantic_warnings = validate_semantics(cfg)
            issues.extend(semantic_errors)

            policy_errors: List[str] = []
            policy_warnings: List[str] = []
            if args.policy == "improvisation1":
                policy_errors, policy_warnings = validate_policy_improvisation1(cfg, p)
                issues.extend(policy_errors)

            all_warnings = list(semantic_warnings) + list(policy_warnings)
            if all_warnings:
                warnings_total += len(all_warnings)
                print(f"\nWARN {p}:")
                for w in all_warnings:
                    print(f"  - {w}")

            if issues:
                failed += 1
                print(f"\n{p}:\n  " + "\n  ".join(issues), file=sys.stderr)

        if failed:
            print(
                f"\nFAILED: {failed}/{len(config_paths)} configs invalid",
                file=sys.stderr,
            )
            return 2

        print(f"OK: {len(config_paths)} configs valid")
        return 0

    except ValidationError as exc:
        print(str(exc), file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
