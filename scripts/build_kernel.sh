#!/usr/bin/env bash
# Build the out-of-tree vinput kernel module.
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
KERNEL_DIR="$ROOT/kernel"

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

if [[ ! -f "$KERNEL_DIR/Makefile" ]]; then
    echo "Error: missing $KERNEL_DIR/Makefile" >&2
    exit 1
fi
if ! command -v make >/dev/null 2>&1; then
    echo 'Error: make is not installed (sudo apt install build-essential).' >&2
    exit 1
fi

# Kernel Makefile supports overriding KDIR in the environment.
headers="${KDIR:-/lib/modules/$(uname -r)/build}"
if [[ ! -f "$headers/Makefile" ]]; then
    echo "Error: kernel build headers not found at $headers" >&2
    echo 'Install matching linux-headers-$(uname -r) or set KDIR.' >&2
    exit 1
fi

cd "$KERNEL_DIR" # Needed: the project Makefile uses M=$(PWD).
if [[ "$mode" == clean || "$mode" == rebuild ]]; then
    echo "Cleaning kernel module in $KERNEL_DIR"
    make clean
fi
if [[ "$mode" == build || "$mode" == rebuild ]]; then
    echo "Building kernel module in $KERNEL_DIR"
    make
    if [[ ! -f vinput.ko ]]; then
        echo 'Error: make finished without producing kernel/vinput.ko' >&2
        exit 1
    fi
    echo "Built: $KERNEL_DIR/vinput.ko"
fi
