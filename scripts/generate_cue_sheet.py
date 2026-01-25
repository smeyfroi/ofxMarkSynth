#!/usr/bin/env python3

"""Update a performance cue sheet from synth config metadata.

This script is intentionally minimal: it reads `performance` metadata from
config JSON files and regenerates only the marked sections in the cue sheet.

Expected markers in the cue sheet:
- <!-- BEGIN GENERATED TOTAL --> ... <!-- END GENERATED TOTAL -->
- <!-- BEGIN GENERATED RUN ORDER --> ... <!-- END GENERATED RUN ORDER -->

Usage (CaldewRiver example):
  python3 generate_cue_sheet.py \
    --configs-dir "/Users/steve/Documents/MarkSynth-performances/CaldewRiver/config/synth" \
    --cue-sheet "/Users/steve/Documents/MarkSynth-performances/CaldewRiver/PERFORMANCE-CUE-SHEET.md"
"""

from __future__ import annotations

import argparse
import json
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any


TOTAL_BEGIN = "<!-- BEGIN GENERATED TOTAL -->"
TOTAL_END = "<!-- END GENERATED TOTAL -->"
RUN_BEGIN = "<!-- BEGIN GENERATED RUN ORDER -->"
RUN_END = "<!-- END GENERATED RUN ORDER -->"


@dataclass(frozen=True)
class CueItem:
    file: str
    order: int
    duration_sec: int
    movement: int | None
    movement_name: str
    section: str
    section_name: str
    role: str
    capture_label: str
    transition_after_sec: int


def parse_duration(duration: str | None) -> int:
    if not duration:
        return 0
    m = re.match(r"^(\d+):(\d+)$", duration.strip())
    if not m:
        return 0
    return int(m.group(1)) * 60 + int(m.group(2))


def format_mmss(seconds: int) -> str:
    m = seconds // 60
    s = seconds % 60
    return f"{m}:{s:02d}"


def load_items(configs_dir: Path) -> list[CueItem]:
    items: list[CueItem] = []
    for fp in sorted(configs_dir.glob("*.json")):
        data = json.loads(fp.read_text())
        perf = data.get("performance")
        if not isinstance(perf, dict):
            continue

        duration_sec = parse_duration(data.get("duration"))
        order = int(perf.get("order", 999))

        capture_label = ""
        cap = perf.get("capture")
        if isinstance(cap, dict) and cap.get("enabled"):
            capture_label = str(cap.get("label", "Capture")).strip()

        transition_after_sec = 0
        transition_after = perf.get("transitionAfter")
        if (
            isinstance(transition_after, dict)
            and transition_after.get("type") == "fadeToBlack"
        ):
            transition_after_sec = int(transition_after.get("durationSec", 0))

        movement = perf.get("movement")
        movement_name = str(perf.get("movementName", "")).strip()
        section = str(perf.get("section", "")).strip()
        section_name = str(perf.get("sectionName", "")).strip()
        role = str(perf.get("role", "")).strip()

        items.append(
            CueItem(
                file=fp.name,
                order=order,
                duration_sec=duration_sec,
                movement=int(movement) if movement is not None else None,
                movement_name=movement_name,
                section=section,
                section_name=section_name,
                role=role,
                capture_label=capture_label,
                transition_after_sec=transition_after_sec,
            )
        )

    items.sort(key=lambda it: it.order)
    return items


def build_run_order_block(items: list[CueItem]) -> tuple[str, int]:
    lines: list[str] = []
    lines.append(
        "Times below are **nominal** and derived from config `duration` plus any planned `transitionAfter` fade-to-black pauses."
    )
    lines.append("")
    lines.append("| At | Dur | Config | Notes | Capture |")
    lines.append("|---:|---:|---|---|---|")

    t = 0
    fade_after: list[tuple[str, int]] = []

    for it in items:
        at = format_mmss(t)
        dur = format_mmss(it.duration_sec)

        # Notes column: keep it short but informative.
        note_bits: list[str] = []
        if it.role == "titleCard":
            note_bits.append("Title card")
        else:
            if it.movement is not None:
                sec = it.section
                # If section is a single-letter scheme (A, B, C, A', A'', A+), keep compact `M3A` style.
                if re.match(r"^[A-Z](?:''|'|\+)?$", sec or ""):
                    # Classic A/B/C-style section labels
                    label = f"M{it.movement}{sec}"
                    if it.section_name:
                        label = f"{label} — {it.section_name}"
                else:
                    # For non-letter sections (e.g. Base, LayerMix), prefer the human name.
                    pretty = it.section_name or sec
                    label = f"M{it.movement}"
                    if pretty:
                        label = f"{label} — {pretty}"

                note_bits.append(label)

        notes = "; ".join(note_bits)

        lines.append(f"| {at} | {dur} | `{it.file}` | {notes} | {it.capture_label} |")

        t += it.duration_sec
        if it.transition_after_sec > 0:
            fade_after.append((it.file, it.transition_after_sec))
            t += it.transition_after_sec

    if fade_after:
        lines.append("")
        lines.append("Planned fade-to-black pauses (manual / operator-driven):")
        for file, sec in fade_after:
            lines.append(f"- After `{file}`: fade to black ~{sec}s")

    return "\n".join(lines) + "\n", t


def replace_block(text: str, begin: str, end: str, replacement_inner: str) -> str:
    if begin not in text or end not in text:
        raise SystemExit(f"Missing markers: {begin} / {end}")

    pattern = re.compile(re.escape(begin) + r".*?" + re.escape(end), re.DOTALL)
    replacement = begin + "\n" + replacement_inner + end
    return pattern.sub(replacement, text, count=1)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--configs-dir", type=Path, required=True)
    ap.add_argument("--cue-sheet", type=Path, required=True)
    args = ap.parse_args()

    items = load_items(args.configs_dir)
    run_order_block, total_sec = build_run_order_block(items)

    cue_text = args.cue_sheet.read_text()

    total_inner = f"**Total nominal duration (incl. planned fade-to-black pauses):** ~{format_mmss(total_sec)}\n"
    cue_text = replace_block(cue_text, TOTAL_BEGIN, TOTAL_END, total_inner)
    cue_text = replace_block(cue_text, RUN_BEGIN, RUN_END, run_order_block)

    args.cue_sheet.write_text(cue_text)


if __name__ == "__main__":
    main()
