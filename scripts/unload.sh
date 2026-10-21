#!/usr/bin/env bash
# Unload the vinput module. Keep uinput unless explicitly requested.
set -euo pipefail

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
            echo 'Error: sudo is required to unload kernel modules.' >&2
            return 1
        fi
        sudo "$@"
    fi
}

if grep -q '^vinput ' /proc/modules; then
    echo 'Unloading vinput...'
    as_root rmmod vinput
    if grep -q '^vinput ' /proc/modules; then
        echo 'Error: vinput is still loaded. Check sudo dmesg.' >&2
        exit 1
    fi
    echo 'Unloaded vinput.'
else
    echo 'vinput is not loaded.'
fi

# uinput is a system module used by other programs. Do not unload by default.
if [[ "${1:-}" == --with-uinput ]]; then
    if grep -q '^uinput ' /proc/modules; then
        echo 'Unloading uinput (explicitly requested)...'
        as_root modprobe -r uinput
        echo 'Unloaded uinput.'
    else
        echo 'uinput is not loaded as a removable module.'
    fi
fi
