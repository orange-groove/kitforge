#!/usr/bin/env python3
"""Patch a broken demo-rock-kit install: bind samples/*.wav into kit.json layers."""

import json
import sys
import uuid
from pathlib import Path

SAMPLE_MAP = {
    ("kick", "Center"): "kick_center.wav",
    ("snare", "Center"): "snare_center.wav",
    ("snare", "Rimshot"): "snare_rimshot.wav",
    ("rackTom", "Hit"): "rack_tom_hit.wav",
    ("floorTom", "Hit"): "floor_tom_hit.wav",
    ("hiHat", "Closed"): "hihat_closed.wav",
    ("hiHat", "Open"): "hihat_open.wav",
    ("crash", "Hit"): "crash_hit.wav",
    ("ride", "Bow"): "ride_bow.wav",
}


def slug(name: str) -> str:
    return name.lower().replace(" ", "_")


def file_for(piece_type: str, art_name: str) -> str:
    key = (piece_type, art_name)
    if key in SAMPLE_MAP:
        return SAMPLE_MAP[key]
    return f"drum_{slug(art_name)}.wav"


def make_layer(relative_path: str) -> dict:
    return {
        "id": uuid.uuid4().hex,
        "minVelocity": 1,
        "maxVelocity": 127,
        "currentRoundRobinIndex": 0,
        "roundRobins": [
            {
                "id": uuid.uuid4().hex,
                "filePath": relative_path,
                "rootMidiNote": 60,
                "gain": 1.0,
                "pan": 0.0,
                "pitch": 1.0,
                "startOffsetSamples": 0,
                "endOffsetSamples": -1,
            }
        ],
    }


def patch(install_root: Path) -> bool:
    kit_json = install_root / "kit.json"
    samples_dir = install_root / "samples"
    if not kit_json.exists() or not samples_dir.is_dir():
        return False

    kit = json.loads(kit_json.read_text())
    changed = False

    for piece in kit.get("pieces", []):
        piece_type = piece.get("type", "")
        for art in piece.get("articulations", []):
            if art.get("layers"):
                continue
            fname = file_for(piece_type, art.get("name", ""))
            if not (samples_dir / fname).exists():
                continue
            art["layers"] = [make_layer(f"samples/{fname}")]
            changed = True

    if not changed:
        return False

    kit_json.write_text(json.dumps(kit, indent=2))
    (install_root / ".kitforge-version").write_text("3.0.2\n")
    return True


if __name__ == "__main__":
    root = Path(sys.argv[1] if len(sys.argv) > 1 else Path.home() / "Documents/KitForge/Libraries/demo-rock-kit")
    ok = patch(root)
    print("patched" if ok else "no changes")
    sys.exit(0 if ok else 1)
