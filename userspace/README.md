# VIF user-space macro recorder and player

## Requirements

- Linux, gcc/cc, make
- The project's shared `include/uapi/vinput_uapi.h` (not bundled here)
- A loaded `vinput.ko` exposing `/dev/vif_driver`
- The patched kernel character driver that reads/writes complete 24-byte `struct vinput_event` records
- Input-handler matching fixed so recording actually receives physical/uinput events

Place this directory at `Virtual-Input-Framework/userspace/` next to `include/`.

## Build

```bash
make -C userspace
```

## Record, inspect, play

```bash
# Driver must be loaded first. Record only when you intend to capture input.
sudo ./userspace/vinput record --output demo.vim --duration 5
sudo ./userspace/vinput info demo.vim
sudo ./userspace/vinput play demo.vim
```

`--device /dev/vif_driver` can override the default device on record/play.
`--max-events N` caps memory and detects overflowing the user-space recording array.
Press Ctrl+C to finish recording early or interrupt playback. **Do not record passwords or other sensitive inputs.** Recording files are mode `0600` and may be root-owned when run under sudo.

### How it works

- `recorder.c`: reads event batches using nonblocking `read()` (the current driver has no `.poll` callback). Filters unsupported events; classifies keyboard keys vs mouse buttons/movements; routes them to virtual device IDs 0 or 1; preserves `SYN_REPORT` for matching source reports; converts timestamps to nanoseconds since the first recorded event.
- `macro.c`: moves your existing macro linked-list registry from kernel allocations (`kzalloc/kfree`) to `calloc/free`; preserves max 256 registry slots, duplicate checks, ordering validation, and delay calculations.
- `player.c`: moves macro playback out of the kernel. Uses `clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, ...)` for absolute timing and writes to the driver's output path; performs release cleanup for held keys/buttons after playback or interruption.
- `macro_file.c`: versioned binary file containing a header and `vinput_event` records.

### Limitations

- The v1 file format is **native-endian and tied to the 24-byte Linux UAPI layout**; use the same architecture/ABI for recording and playback.
- Only supports the current virtual devices' keyboard key codes, BTN_LEFT/RIGHT/MIDDLE, REL_X/REL_Y, and SYN_REPORT. Events such as scroll wheel and absolute axes are skipped.
- The input capture interface currently has a shared single-consumer ring buffer and no session-based START/STOP ioctl. Other consumers can steal events; input can be captured even when the recorder is not running, and the kernel buffer may overflow during recording.
- Pressing Ctrl+C during playback emits key/button releases on a best-effort basis. SIGKILL or a driver failure can prevent cleanup.
- This is basic duration-bounded recording, not perfect physical-device identification. Device IDs assigned by the input handler are ephemeral. Macros store virtual output destinations instead.
- No special kernel macro ioctl is needed. Once the user-space program is built and tested, remove `vinput_macro.o` from **kernel/Makefile** and ensure no kernel source references `vinput_macro.h`.
