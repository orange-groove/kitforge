#!/usr/bin/env bash
# Fetch CC0 drum one-shots from stargate-sample-pack (see Resources/demo-samples/SOURCES.md).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/Resources/demo-samples"
TMP="$OUT/.fetch_tmp"
BASE="https://raw.githubusercontent.com/stargatedaw/stargate-sample-pack/main/stargate-sample-pack/fugue-state-audio/drums"

mkdir -p "$TMP" "$OUT"

fetch() {
  local src="$1"
  local dest="$2"
  echo "Fetching $src -> $dest"
  curl -fsSL "$BASE/$src" -o "$TMP/$(basename "$src")"
  ffmpeg -y -loglevel error -i "$TMP/$(basename "$src")" -ar 44100 -ac 1 "$OUT/$dest"
}

# Cohesive fugue-state-audio "distkit" set (CC0 via stargate-sample-pack)
fetch "kicks/distkit-kick.wav"           kick_center.wav
fetch "snares/distkit-snare.wav"         snare_center.wav
fetch "percussion/x0xproc2-rimshot.wav"  snare_rimshot.wav
fetch "toms/distkit-midtom.wav"         rack_tom_hit.wav
fetch "toms/distkit-lotom.wav"           floor_tom_hit.wav
fetch "hihats/distkit-hatclsd.wav"       hihat_closed.wav
fetch "hihats/distkit-hatopen.wav"       hihat_open.wav
fetch "cymbals/distkit-crash.wav"        crash_hit.wav
fetch "cymbals/distkit-ride.wav"         ride_bow.wav

rm -rf "$TMP"
echo "Done. Wrote samples to $OUT"
