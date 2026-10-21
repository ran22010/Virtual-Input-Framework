#!/usr/bin/env bash
# Insert kernel/vinput.ko; optionally load uinput for capture tests.
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
MODULE_FILE="$ROOT/kernel/vinput.ko"

usage() {
    echo "Usage: $0 [--with-uinput]" >&2
}

if (( $# > 1 )); then usage; exit 2; fi
case "${1:-}" in
    ''|--with-uinput) ;;
    -h|--help) usage; exit 0 ;;
    *) usage; exit 2 ;;
esac

as_root() {
    if (( EUID == 0 )); then
        "$@"
    else
        if ! command -v sudo >/dev/null 2>&1; then
            echo 'Error: sudo is required to load kernel modules.' >&2
            return 1
        fi
        sudo "$@"
    fi
}

if [[ ! -f "$MODULE_FILE" ]]; then
    echo "Error: $MODULE_FILE does not exist. Run scripts/build_kernel.sh first." >&2
    exit 1
fi
if grep -q '^vinput ' /proc/modules; then
    echo 'Error: vinput is already loaded. Unload it with scripts/unload.sh before loading a new build.' >&2
    exit 1
fi

if [[ "${1:-}" == --with-uinput ]]; then
    echo 'Loading optional uinput module for capture tests...'
    as_root modprobe uinput
fi

echo "Loading $MODULE_FILE"
as_root insmod "$MODULE_FILE"
if ! grep -q '^vinput ' /proc/modules; then
    echo 'Error: vinput is not listed in /proc/modules after insmod.' >&2
    exit 1
fi

echo 'Loaded vinput.'
if [[ -e /dev/vif_driver ]]; then
    ls -l /dev/vif_driver
else
    echo 'Warning: /dev/vif_driver not present yet. Check udev and sudo dmesg.' >&2
fi
