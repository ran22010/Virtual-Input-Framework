# Userspace Utilities (`userspace/`)

## Overview
This directory contains the userland tools for controlling, testing, and benchmarking the Virtual Input Framework. These utilities interact directly with `/dev/vinput` via `ioctl`, standard file I/O, and `/dev/input/eventX` streams.

## Sub-Modules
* **`config/` (`vinput-config`)**: 
  * **Role:** Command-line management tool to dynamically configure driver modes, register virtual devices, and upload macro definitions to kernel space.
* **`record/` (`vinput-record`)**: 
  * **Role:** Live input capture utility that listens to existing input node streams (`/dev/input/eventX`) to record macros or log input streams for replay.
* **`bench/` (`vinput-bench`)**: 
  * **Role:** Latency and throughput benchmarking tool. Injects high-frequency event batches into `/dev/vinput` and reads telemetry to measure jitter, CPU overhead, and queue drop rates.

## Building
Build all userspace binaries using CMake or the provided top-level Makefile:

```bash
mkdir build && cd build
cmake ..
make
```