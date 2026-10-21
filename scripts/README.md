# Virtual Input Framework helper scripts

Put this `scripts/` directory in the **repository root**, alongside `kernel/` and `test/`.
Run commands from anywhere; each script locates the repo using its own path.

| Command | Action |
| --- | --- |
| `./scripts/build_kernel.sh` | `make` in `kernel/` and verify `vinput.ko` exists |
| `./scripts/build_tests.sh` | `make` in `test/` |
| `./scripts/build_all.sh` | Build kernel module, then test binaries |
| `./scripts/load.sh` | `insmod kernel/vinput.ko` (requires root/sudo) |
| `./scripts/load.sh --with-uinput` | Also run `modprobe uinput` for the capture tests |
| `./scripts/unload.sh` | `rmmod vinput` (requires root/sudo) |
| `./scripts/unload.sh --with-uinput` | Also try to remove `uinput` (only if explicitly requested) |

For build scripts, use `clean` or `rebuild` as a positional argument, e.g. `./scripts/build_all.sh rebuild`.
The scripts **do not** automatically unload a loaded module before rebuilding, or replace a loaded module. Unload first, then rebuild and load:

```bash
./scripts/unload.sh
./scripts/build_all.sh rebuild
./scripts/load.sh --with-uinput
sudo ./test/run_tests.sh
./scripts/unload.sh
```

The test runner is provided separately in `test/run_tests.sh`; these scripts don't run the tests automatically. `uinput` is a shared Linux module, so normal unload leaves it loaded rather than removing a dependency that other programs could be using.

Requirements: Linux, Bash, `make`, matching kernel headers, a C compiler, and `kmod` (`insmod`/`rmmod`/`modprobe`). The user-space tests use the repository's existing `uapi/vinput_uapi.h`.

If the kernel build fails with missing headers, on Ubuntu install `linux-headers-$(uname -r)`. If you need a different headers tree, export `KDIR=/path/to/kernel/build` before running the build command.
