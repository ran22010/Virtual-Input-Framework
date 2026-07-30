# Userspace API (UAPI) Headers (`include/uapi/`)

## Overview
This directory contains header files defining the User API boundary between userspace utilities and the kernel driver. 

Following standard Linux Kernel conventions (similar to `/include/uapi` in the kernel tree), these headers define shared structures, magic numbers, and `ioctl` command interface codes used by both sides.

## Files
* **`vinput_uapi.h`**: Primary API header containing:
  * `ioctl` request macros constructed using `_IO()`, `_IOW()`, `_IOR()`, and `_IOWR()`.
  * Common fixed-width data structures (e.g., `struct vinput_event`, `struct vinput_stats_info`, `struct vinput_macro_rule`).
  * Virtual device configuration flags and feature definitions.

## Design Rules
* **No Kernel-Internal Types:** Use Linux fixed-width types (e.g., `__u32`, `__u64`) or C standard types (`uint32_t`, `uint64_t`) to ensure memory layout parity across userspace and kernel space.
* **Strict Packing & Alignment:** Keep structures 64-bit aligned to avoid padding differences between 32-bit and 64-bit execution environments.