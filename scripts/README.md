# Automation & System Scripts (`scripts/`)

## Overview
Helper scripts for environment setup, kernel module lifecycle control, and system permission management.

## Contents
* **`load.sh`**: 
  * Inserts the `vinput.ko` kernel module via `insmod`.
  * Dynamically parses `/proc/devices` for the major device number.
  * Creates the character device node (`/dev/vinput`) using `mknod`.
  * Applies group read/write permissions for non-root testing.
* **`unload.sh`**: 
  * Safely cleans up the `/dev/vinput` device node.
  * Removes the module using `rmmod`.
* **`99-vinput.rules`**: 
  * Custom `udev` configuration rule to automatically set proper device permissions and user group membership (`input` group) whenever `/dev/vinput` is created.

## Usage

```bash
# Load module and initialize device node
sudo ./scripts/load.sh

# Unload module and teardown node
sudo ./scripts/unload.sh
```