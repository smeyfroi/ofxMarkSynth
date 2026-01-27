#!/usr/bin/env python3

"""Validate MarkSynth synth configs for structural consistency.

This is a lightweight validator intended to catch the most common drift issues
while iterating on performance configs:

- Connections referencing mods that don't exist in `mods`
- `layers` references to drawing layers that don't exist in `drawingLayers`
- Orphan mods (defined but neither connected nor drawing)

It does *not* currently validate port names (sources/sinks) against mod types,
which would require a runtime mod registry.

Usage:
  python3 validate_synth_configs.py --configs-dir /path/to/config/synth

Exit code:
- 0: no issues
- 1: validation issues found
"""

from __future__ import annotations

import argparse
import json
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Issue:
    file: str
    kind: str
    detail: str


def has_any_layers(mod_cfg: object) -> bool:
    """Return true if the mod has any drawing layer target configured."""
    if not isinstance(mod_cfg, dict):
        return False
    layers = mod_cfg.get("layers")
    if not isinstance(layers, dict):
        return False
    for arr in layers.values():
        if isinstance(arr, list) and any(isinstance(x, str) for x in arr):
            return True
    return False


def parse_connection_endpoint(endpoint: str) -> str:
    """Return mod name for `Mod.Port` or empty for `.Port`."""
    endpoint = endpoint.strip()
    if not endpoint:
        return ""
    # Expected forms: "Mod.Port" or ".Port"
    if endpoint.startswith("."):
        return ""  # Synth
    if "." not in endpoint:
        # malformed; treat entire token as "mod"
        return endpoint
    return endpoint.split(".", 1)[0].strip()


def validate_file(fp: Path, *, ignore_orphan_mods: bool) -> list[Issue]:
    issues: list[Issue] = []
    try:
        data = json.loads(fp.read_text())
    except Exception as e:
        return [Issue(fp.name, "json", f"Failed to parse JSON: {e}")]

    mods = data.get("mods")
    if not isinstance(mods, dict):
        issues.append(
            Issue(fp.name, "structure", "Missing or invalid top-level `mods` object")
        )
        mods = {}

    drawing_layers = data.get("drawingLayers")
    if not isinstance(drawing_layers, dict):
        issues.append(
            Issue(
                fp.name,
                "structure",
                "Missing or invalid top-level `drawingLayers` object",
            )
        )
        drawing_layers = {}

    mod_names = set(mods.keys())
    layer_names = set(drawing_layers.keys())

    # 1) Dangling mod references in connections
    conns = data.get("connections")
    if conns is None:
        conns = []
    if not isinstance(conns, list):
        issues.append(Issue(fp.name, "structure", "`connections` is not a list"))
        conns = []

    referenced_mods: set[str] = set()
    for c in conns:
        if not isinstance(c, str):
            issues.append(
                Issue(fp.name, "connections", f"Non-string connection entry: {c!r}")
            )
            continue
        if "->" not in c:
            issues.append(
                Issue(
                    fp.name,
                    "connections",
                    f"Malformed connection (missing '->'): {c!r}",
                )
            )
            continue
        left, right = [s.strip() for s in c.split("->", 1)]
        src_mod = parse_connection_endpoint(left)
        dst_mod = parse_connection_endpoint(right)
        if src_mod:
            referenced_mods.add(src_mod)
        if dst_mod:
            referenced_mods.add(dst_mod)

    missing_mods = sorted([m for m in referenced_mods if m not in mod_names])
    if missing_mods:
        issues.append(
            Issue(
                fp.name,
                "connections",
                f"Missing referenced mods: {', '.join(missing_mods)}",
            )
        )

    # 1b) Orphan mods: defined but neither connected nor drawing.
    # These are usually leftovers after refactors (e.g. removing wiring or layers).
    if not ignore_orphan_mods:
        orphan_mods = sorted(
            [
                name
                for name, cfg in mods.items()
                if name not in referenced_mods and not has_any_layers(cfg)
            ]
        )
        if orphan_mods:
            issues.append(
                Issue(
                    fp.name,
                    "mods",
                    "Orphan mods (no connections and no layers): "
                    + ", ".join(orphan_mods),
                )
            )

    # 2) Missing layer references
    for mod_name, mod_cfg in mods.items():
        if not isinstance(mod_cfg, dict):
            continue
        layers = mod_cfg.get("layers")
        if layers is None:
            continue
        if not isinstance(layers, dict):
            issues.append(
                Issue(fp.name, "layers", f"`{mod_name}.layers` is not an object")
            )
            continue
        for group_name, arr in layers.items():
            if not isinstance(arr, list):
                issues.append(
                    Issue(
                        fp.name,
                        "layers",
                        f"`{mod_name}.layers.{group_name}` is not a list",
                    )
                )
                continue
            for layer in arr:
                if not isinstance(layer, str):
                    issues.append(
                        Issue(
                            fp.name,
                            "layers",
                            f"`{mod_name}.layers.{group_name}` contains non-string: {layer!r}",
                        )
                    )
                    continue
                if layer not in layer_names:
                    issues.append(
                        Issue(
                            fp.name,
                            "layers",
                            f"`{mod_name}.layers.{group_name}` references missing drawingLayer `{layer}`",
                        )
                    )

    return issues


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--configs-dir", type=Path, required=True)
    ap.add_argument(
        "--ignore-orphan-mods",
        action="store_true",
        help="Skip checking for mods that are neither connected nor drawing",
    )
    args = ap.parse_args()

    all_issues: list[Issue] = []
    for fp in sorted(args.configs_dir.glob("*.json")):
        all_issues.extend(validate_file(fp, ignore_orphan_mods=args.ignore_orphan_mods))

    if not all_issues:
        print(
            f"OK: {args.configs_dir} ({len(list(args.configs_dir.glob('*.json')))} files)"
        )
        raise SystemExit(0)

    by_file: dict[str, list[Issue]] = {}
    for issue in all_issues:
        by_file.setdefault(issue.file, []).append(issue)

    print(f"Found {len(all_issues)} issue(s) in {len(by_file)} file(s):")
    for file in sorted(by_file.keys()):
        print(f"\n{file}")
        for issue in by_file[file]:
            print(f"  - {issue.kind}: {issue.detail}")

    raise SystemExit(1)


if __name__ == "__main__":
    main()
