# Testing & Verification (`tests/`)

## Overview
Unit tests, concurrency stress tests, and automated integration scripts used to validate driver stability, boundary safety, and performance.

## Contents
* **`test_ringbuf.c`**: 
  * Standalone test suite designed to evaluate the lockless circular ring buffer logic under multi-threaded contention to ensure thread safety and boundary correctness.
* **`integration_test.sh`**: 
  * End-to-end automation test script. Loads the kernel module, uses `vinput-config` to register a virtual keyboard/mouse, injects test events, and verifies output against synthetic event readings.

## Running Tests

```bash
# Run unit tests
gcc -O2 test_ringbuf.c -o test_ringbuf && ./test_ringbuf

# Run full integration test (requires root for kernel module load)
sudo ./integration_test.sh
```