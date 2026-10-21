#!/usr/bin/env bash
# Build the user-space macro recorder/player.
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
USERSPACE_DIR="$ROOT/userspace"

mode="${1:-build}"

case "$mode" in
    build|clean|rebuild) ;;
    *)
        echo "Usage: $0 [build|clean|rebuild]" >&2
        exit 2
        ;;
esac

if [[ ! -f "$USERSPACE_DIR/Makefile" ]]; then
    echo "Error: missing $USERSPACE_DIR/Makefile" >&2
    exit 1
fi

if [[ "$mode" == clean || "$mode" == rebuild ]]; then
    make -C "$USERSPACE_DIR" clean
fi

if [[ "$mode" == build || "$mode" == rebuild ]]; then
    make -C "$USERSPACE_DIR"
    echo "Built: $USERSPACE_DIR/vinput"
fi