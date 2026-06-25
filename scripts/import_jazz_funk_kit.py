#!/usr/bin/env python3
"""
Legacy one-off script — prefer KitForge in-app Import Kontakt... (KontaktImporter).

Imports Native Instruments-style Jazz Funk Kit samples into KitForge native format.

Naming:  "{instrument} - {optional part} - {layer}.wav"
  layer 1-9  → velocity layers OR round-robin (if high layers 91+ exist)
  layer 91+  → explicit high-velocity layers

Output:
  <out>/kitforge/kit.json
  <out>/kitforge/samples/*.wav
  <out>/kitforge/license.txt
  <out>/kitforge/credits.txt
  <out>/install.json
"""

from __future__ import annotations

import json
import re
import shutil
import uuid
from collections import defaultdict
from pathlib import Path

SOURCE = Path.home() / "Downloads" / "Jazz Funk Kit" / "samples"
OUT = Path.home() / "Documents" / "KitForge" / "Libraries" / "jazz-funk-kit"
KIT_ID = "jazz-funk-kit"
KIT_NAME = "Jazz Funk Kit"

SKIP_PATTERNS = re.compile(r"^turn (snare on|off snares)", re.I)

# (piece_key, piece_name, piece_type) -> articulation_key -> (art_name, midi, choke)
PIECE_MAP = {
    "kick": ("kick", "Kick", "kick"),
    "bop kick": ("bop_kick", "Bop Kick", "kick"),
    "snare": ("snare", "Snare", "snare"),
    "rimshot": ("snare", "Snare", "snare"),
    "stickshot": ("snare", "Snare", "snare"),
    "xstick": ("snare", "Snare", "snare"),
    "rack tom": ("rack_tom", "Rack Tom", "rackTom"),
    "floor tom": ("floor_tom", "Floor Tom", "floorTom"),
    "hihat": ("hihat", "Hi-Hat", "hiHat"),
    "ride": ("ride", "Ride", "ride"),
    "flat ride": ("flat_ride", "Flat Ride", "ride"),
}

ART_MAP = {
    ("kick", "snares off"): ("Center (Snares Off)", 36, ""),
    ("kick", "snares on"): ("Center (Snares On)", 36, ""),
    ("bop kick", "snares off"): ("Bop (Snares Off)", 35, ""),
    ("bop kick", "snares on"): ("Bop (Snares On)", 35, ""),
    ("snare", "snares off"): ("Center (Snares Off)", 38, ""),
    ("snare", "snares on"): ("Center (Snares On)", 38, ""),
    ("rimshot", "snares off"): ("Rimshot (Snares Off)", 40, ""),
    ("rimshot", "snares on"): ("Rimshot (Snares On)", 40, ""),
    ("stickshot", "snares off"): ("Stickshot (Snares Off)", 38, ""),
    ("stickshot", "snares on"): ("Stickshot (Snares On)", 38, ""),
    ("xstick", "snares off"): ("Cross Stick (Snares Off)", 37, ""),
    ("xstick", "snares on"): ("Cross Stick (Snares On)", 37, ""),
    ("rack tom", "snares off"): ("Hit (Snares Off)", 48, ""),
    ("rack tom", "snares on"): ("Hit (Snares On)", 48, ""),
    ("floor tom", "snares off"): ("Hit (Snares Off)", 43, ""),
    ("floor tom", "snares on"): ("Hit (Snares On)", 43, ""),
    ("hihat", "close"): ("Closed Tight", 42, "hihat"),
    ("hihat", "closed"): ("Closed", 42, "hihat"),
    ("hihat", "closed side"): ("Closed Side", 42, "hihat"),
    ("hihat", "open"): ("Open", 46, "hihat"),
    ("hihat", "opened 1"): ("Opened 1", 46, "hihat"),
    ("hihat", "opened 2"): ("Opened 2", 46, "hihat"),
    ("hihat", "opened 3"): ("Opened 3", 46, "hihat"),
    ("hihat", "opened 4"): ("Opened 4", 46, "hihat"),
    ("hihat", "opened 5"): ("Opened 5", 46, "hihat"),
    ("ride", ""): ("Bow", 51, ""),
    ("ride", "bell"): ("Bell", 53, ""),
    ("ride", "crash"): ("Crash", 59, ""),
    ("flat ride", ""): ("Bow", 51, ""),
    ("flat ride", "crash"): ("Crash", 59, ""),
}


def make_id() -> str:
    return str(uuid.uuid4())


def parse_filename(name: str) -> tuple[str, str, int] | None:
    base = name.replace(".wav", "").strip()
    if SKIP_PATTERNS.search(base):
        return None

    m = re.match(r"^(.+?) - (.+?) - (\d+)$", base)
    if m:
        return m.group(1).strip().lower(), m.group(2).strip().lower(), int(m.group(3))

    m = re.match(r"^(.+?) - (\d+)$", base)
    if m:
        return m.group(1).strip().lower(), "", int(m.group(2))

    return None


def velocity_for_layer(layer: int, layers: list[int]) -> tuple[int, int, bool]:
    """Returns (min_vel, max_vel, is_round_robin_group)."""
    layers = sorted(set(layers))
    high = [n for n in layers if n >= 10]
    low = [n for n in layers if n < 10]

    if layer >= 10:
        return max(1, layer - 2), min(127, layer + 2), False

    if high:
        # Low layers are round-robin in the main body (1-90)
        return 1, 90, True

    # Pure velocity layering (e.g. kick 1-5)
    count = len(low)
    idx = low.index(layer)
    span = 127 // count
    lo = idx * span + 1
    hi = 127 if idx == count - 1 else (idx + 1) * span
    return lo, hi, False


def main() -> None:
    if not SOURCE.is_dir():
        raise SystemExit(f"Source not found: {SOURCE}")

    wavs = sorted(SOURCE.glob("*.wav"))
    print(f"Found {len(wavs)} WAV files")

    # art_key -> layer_num -> [files]
    groups: dict[tuple, dict[int, list[Path]]] = defaultdict(lambda: defaultdict(list))

    for wav in wavs:
        parsed = parse_filename(wav.name)
        if not parsed:
            print(f"  skip: {wav.name}")
            continue
        instrument, part, layer = parsed
        if instrument not in PIECE_MAP:
            print(f"  unknown instrument: {wav.name}")
            continue
        piece_key, _, _ = PIECE_MAP[instrument]
        art_key = (piece_key, instrument, part)
        groups[art_key][layer].append(wav)

    kitforge = OUT / "kitforge"
    samples_out = kitforge / "samples"
    if OUT.exists():
        shutil.rmtree(OUT)
    samples_out.mkdir(parents=True)

    pieces: dict[str, dict] = {}

    def get_piece(piece_key: str, display: str, ptype: str) -> dict:
        if piece_key not in pieces:
            pieces[piece_key] = {
                "id": make_id(),
                "name": display,
                "type": ptype,
                "primaryMidiNote": 36,
                "chokeGroupId": "hihat" if ptype == "hiHat" else "",
                "volume": 1.0,
                "pan": 0.0,
                "pitch": 1.0,
                "muted": False,
                "soloed": False,
                "x": 100.0,
                "y": 100.0,
                "width": 90.0,
                "height": 90.0,
                "rotation": 0.0,
                "color": "#ffffff",
                "shapeType": "rectangle" if ptype == "kick" else "circle",
                "articulations": [],
            }
        return pieces[piece_key]

    sample_count = 0

    for (piece_key, instrument, part), layer_files in sorted(groups.items()):
        _, display, ptype = PIECE_MAP[instrument]
        piece = get_piece(piece_key, display, ptype)

        art_info = ART_MAP.get((instrument, part)) or ART_MAP.get((instrument, ""))
        if not art_info:
            art_name = part.title() if part else "Hit"
            midi = 36
            choke = "hihat" if ptype == "hiHat" else ""
        else:
            art_name, midi, choke = art_info

        art = {
            "id": make_id(),
            "name": art_name,
            "midiNote": midi,
            "chokeGroupId": choke,
            "layers": [],
        }

        all_layer_nums = sorted(layer_files.keys())
        # Group into sample layers by velocity range
        layer_buckets: dict[tuple[int, int], list[tuple[int, Path]]] = defaultdict(list)

        for layer_num in all_layer_nums:
            lo, hi, is_rr = velocity_for_layer(layer_num, all_layer_nums)
            for f in layer_files[layer_num]:
                layer_buckets[(lo, hi)].append((layer_num, f))

        for (lo, hi), entries in sorted(layer_buckets.items()):
            entries.sort(key=lambda x: x[0])
            sl = {
                "id": make_id(),
                "minVelocity": lo,
                "maxVelocity": hi,
                "currentRoundRobinIndex": 0,
                "roundRobins": [],
            }
            for layer_num, src in entries:
                safe_name = re.sub(r"[^\w\-]+", "_", src.stem.lower()) + ".wav"
                dest = samples_out / safe_name
                if dest.exists():
                    dest = samples_out / f"{layer_num}_{safe_name}"
                shutil.copy2(src, dest)
                sl["roundRobins"].append(
                    {
                        "id": make_id(),
                        "filePath": f"samples/{dest.name}",
                        "rootMidiNote": midi,
                        "gain": 1.0,
                        "pan": 0.0,
                        "pitch": 1.0,
                        "startOffsetSamples": 0,
                        "endOffsetSamples": -1,
                    }
                )
                sample_count += 1
            art["layers"].append(sl)

        piece["articulations"].append(art)
        if not piece.get("_midi_set"):
            piece["primaryMidiNote"] = midi
            piece["_midi_set"] = True

    # Layout pieces on canvas
    layout = {
        "kick": (0.12, 0.72),
        "bop_kick": (0.22, 0.72),
        "snare": (0.38, 0.55),
        "rack_tom": (0.48, 0.42),
        "floor_tom": (0.58, 0.62),
        "hihat": (0.28, 0.38),
        "ride": (0.72, 0.35),
        "flat_ride": (0.82, 0.45),
    }
    for pk, piece in pieces.items():
        piece.pop("_midi_set", None)
        if pk in layout:
            piece["x"] = layout[pk][0] * 900
            piece["y"] = layout[pk][1] * 520
        piece["midiNotes"] = sorted({a["midiNote"] for a in piece["articulations"]})

    kit = {
        "version": 2,
        "kitName": KIT_NAME,
        "pieces": list(pieces.values()),
    }

    kitforge.mkdir(parents=True, exist_ok=True)
    (kitforge / "kit.json").write_text(json.dumps(kit, indent=2))
    (kitforge / "license.txt").write_text(
        "Jazz Funk Kit — see original License Agreement.pdf in source folder.\n"
        "Imported for personal use via KitForge.\n"
    )
    (kitforge / "credits.txt").write_text(
        f"Kit: {KIT_NAME}\n"
        f"Imported from Native Instruments Kontakt library.\n"
        f"Samples: {sample_count} WAV files\n"
        f"Generated by scripts/import_jazz_funk_kit.py\n"
    )

    install = {
        "id": KIT_ID,
        "name": KIT_NAME,
        "format": "kitforgepack",
        "license": "see source License Agreement.pdf",
        "installedPath": str(OUT),
        "sourcePath": str(SOURCE.parent),
        "kitForgePath": str(kitforge),
        "sfzFileUsed": "",
        "sampleCount": sample_count,
        "pieceCount": len(pieces),
        "installedAtMs": 0,
        "tags": ["jazz", "funk", "acoustic", "multi-velocity"],
    }
    (OUT / "install.json").write_text(json.dumps(install, indent=2))

    print(f"\nKitForge kit written to: {kitforge}")
    print(f"  Pieces: {len(pieces)}")
    print(f"  Articulations: {sum(len(p['articulations']) for p in pieces.values())}")
    print(f"  Samples copied: {sample_count}")
    print(f"  install.json: {OUT / 'install.json'}")


if __name__ == "__main__":
    main()
