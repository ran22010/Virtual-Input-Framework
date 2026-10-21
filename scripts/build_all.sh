#!/usr/bin/env bash
# Build the kernel module, tests, and user-space application.
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"

"$SCRIPT_DIR/build_kernel.sh" "${1:-build}"
"$SCRIPT_DIR/build_tests.sh" "${1:-build}"
"$SCRIPT_DIR/build_userspace.sh" "${1:-build}"