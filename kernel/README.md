# Kernel Module Source (`kernel/`)

## Overview
This folder contains the source code for the Linux Kernel Module (LKM) character device driver (`/dev/vinput`). The driver manages event queuing, state-machine processing, rate limiting, and virtual input synthesis via the Linux `uinput` kernel subsystem.

## Architecture & File Layout
* **`vinput_main.c`**: Module entry/exit points (`module_init`, `module_exit`), dynamic character device allocation, and `file_operations` (`open`, `release`, `read`, `write`, `unlocked_ioctl`).
* **`vinput_ringbuf.c` / `.h`**: High-performance, lockless circular ring buffer implementation for thread-safe event queuing without kernel allocations in critical paths.
* **`vinput_macro.c` / `.h`**: Kernel-space state machine for evaluating macro triggers and expanding inputs into multi-event sequences.
* **`vinput_sched.c` / `.h`**: Event scheduler utilizing high-resolution timers (`hrtimer`) and kernel workqueues for rate limiting and paced injection.
* **`vinput_stats.c` / `.h`**: Atomic telemetry logic maintaining real-time statistics exposed via `ioctl` and `procfs` (`/proc/vinput_stats`).
* **`vinput_uinput.c` / `.h`**: Integration layer for creating and managing virtual input devices (`input_dev`) using kernel `uinput` interfaces.

## Building
Compile the kernel module against your running kernel headers:
```bash
make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules