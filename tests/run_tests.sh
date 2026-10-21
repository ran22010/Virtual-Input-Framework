#!/usr/bin/env bash
set -uo pipefail
cd "$(dirname "$0")"

if [[ ! -e /dev/vif_driver ]]; then
  echo "ERROR: /dev/vif_driver not found. Build and load kernel/vinput.ko first." >&2
  exit 1
fi

passed=0 failed=0 skipped=0
for test in test_vinput_io test_vinput_output test_vinput_capture; do
  printf '\n===== %s =====\n' "$test"
  "./$test"
  rc=$?
  if (( rc == 0 )); then
    ((passed+=1))
  elif (( rc == 77 )); then
    ((skipped+=1))
  else
    ((failed+=1))
  fi
done
printf '\nTest binaries: %d passed, %d failed, %d skipped\n' "$passed" "$failed" "$skipped"
(( failed == 0 ))
