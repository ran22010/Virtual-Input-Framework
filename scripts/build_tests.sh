#!/usr/bin/env bash
# Build the integration tests under test/.
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
TEST_DIR="$ROOT/tests"

usage() {
    echo "Usage: $0 [build|clean|rebuild]" >&2
}

if (( $# > 1 )); then usage; exit 2; fi
mode="${1:-build}"
case "$mode" in
    build|clean|rebuild) ;;
    -h|--help) usage; exit 0 ;;
    *) usage; exit 2 ;;
esac

if [[ ! -f "$TEST_DIR/Makefile" ]]; then
    echo "Error: missing $TEST_DIR/Makefile" >&2
    exit 1
fi
if ! command -v make >/dev/null 2>&1; then
    echo 'Error: make is not installed (sudo apt install build-essential).' >&2
    exit 1
fi

cd "$TEST_DIR"
if [[ "$mode" == clean || "$mode" == rebuild ]]; then
    echo "Cleaning tests in $TEST_DIR"
    make clean
fi
if [[ "$mode" == build || "$mode" == rebuild ]]; then
    echo "Building tests in $TEST_DIR"
    make
    echo 'Test binaries built.'
fi
