#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IMAGE_NAME="ofxmarksynth-config-validator:local"

usage() {
  cat <<'EOF'
Usage:
  ./run.sh <path-to-config-or-directory> [--strict] [--policy none|improvisation1]

Examples:
  ./run.sh /Users/steve/Documents/MarkSynth-performances/Improvisation1/config/synth
  ./run.sh /Users/steve/Documents/MarkSynth-performances/Improvisation1/config/synth/00-minimal-av-softmarks.json
  ./run.sh /Users/steve/Documents/MarkSynth-performances/Improvisation1/config/synth --policy improvisation1

Notes:
- This script builds a local Docker image (Python 3.12) and runs the validator.
- The validator mounts the target path read-only at `/input`.
- The ofxMarkSynth addon is mounted read-only at `/ofx` for source/sink extraction.
EOF
}

if [[ ${1:-} == "" || ${1:-} == "-h" || ${1:-} == "--help" ]]; then
  usage
  exit 0
fi

TARGET_PATH="$1"
shift
EXTRA_ARGS=("$@")

TARGET_ABS="$(python3 -c 'import os,sys; print(os.path.abspath(sys.argv[1]))' "$TARGET_PATH")"

# Build image
docker build -t "$IMAGE_NAME" "$SCRIPT_DIR" 1>/dev/null

# Compute addon root for C++ map extraction
ADDON_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Mount target into container at /input
if [[ -d "$TARGET_ABS" ]]; then
  TARGET_MOUNT_SRC="$TARGET_ABS"
  CONTAINER_PATH="/input"
elif [[ -f "$TARGET_ABS" ]]; then
  TARGET_MOUNT_SRC="$(dirname "$TARGET_ABS")"
  CONTAINER_PATH="/input/$(basename "$TARGET_ABS")"
else
  echo "Path not found: $TARGET_ABS" >&2
  exit 2
fi

docker run --rm \
  -v "$ADDON_ROOT:/ofx:ro" \
  -v "$TARGET_MOUNT_SRC:/input:ro" \
  "$IMAGE_NAME" "${EXTRA_ARGS[@]}" --ofxmarksynth-root /ofx "$CONTAINER_PATH"
