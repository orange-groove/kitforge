#!/usr/bin/env bash
# Fast dev build: Ninja + Standalone only (no AU/VST3 install).
#
# Usage:
#   ./scripts/dev-build.sh              # Debug (default)
#   ./scripts/dev-build.sh release    # Release
#   ./scripts/dev-build.sh run          # Debug build + launch app
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

if ! command -v ninja >/dev/null 2>&1; then
    echo "ninja not found. Install with: brew install ninja" >&2
    exit 1
fi

MODE="${1:-debug}"
RUN_AFTER=false

if [[ "$MODE" == "run" ]]; then
    RUN_AFTER=true
    MODE="debug"
fi

case "$MODE" in
    debug|release) ;;
    *)
        echo "Usage: $0 [debug|release|run]" >&2
        exit 1
        ;;
esac

CONFIGURE_PRESET="ninja-${MODE}"
BUILD_PRESET="standalone-${MODE}"
BUILD_TYPE="$(echo "$MODE" | awk '{print toupper(substr($0,1,1)) substr($0,2)}')"

echo "→ cmake --preset ${CONFIGURE_PRESET}"
cmake --preset "${CONFIGURE_PRESET}"

echo "→ cmake --build --preset ${BUILD_PRESET}"
cmake --build --preset "${BUILD_PRESET}"

APP="build-ninja/KitForge_artefacts/${BUILD_TYPE}/Standalone/KitForge.app"

echo ""
echo "Built: ${APP}"

if [[ "$RUN_AFTER" == true ]]; then
    open "${APP}"
fi
