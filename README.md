# Virtual Input Framework

A Linux kernel module and userspace application for capturing, recording, and replaying keyboard and mouse input events.

The project uses the Linux Input Subsystem to intercept input events and `uinput` to generate virtual keyboard and mouse events.

## Features

- **Input Capture:** Capture keyboard and mouse events through a custom Linux `input_handler`.
- **Virtual Input Devices:** Generate keyboard and mouse events using Linux `uinput`.
- **Event Buffering:** Transfer events between kernel space and userspace using thread-safe ring buffers.
- **Macro Recording:** Record input events and preserve their relative timing.
- **Macro Playback:** Replay recorded events through virtual input devices.
- **Macro Persistence:** Save and load macros using a versioned binary file format.
- **Integration Tests:** Test device I/O, virtual output, and input capture.

## Architecture

The framework consists of two primary components.

### Kernel Module

The kernel module manages physical input capture, event buffering, and virtual input generation.

- `vinput_main.c` — Character device interface (`/dev/vif_driver`), initialization, and cleanup.
- `vinput_input.c` — Input handler for capturing Linux input events.
- `vinput_ringbuf.c` — Thread-safe event ring buffers.
- `vinput_virtual.c` — Virtual keyboard and mouse devices and event playback worker.

### Userspace Application

The userspace application manages macro recording, storage, and playback.

- `main.c` — Command-line interface.
- `macro.c` — Macro data structures and management.
- `recorder.c` — Recording events from the kernel driver.
- `player.c` — Replaying recorded events with timing.
- `macro_file.c` — Saving and loading macro files.

### Data Flow

**Recording**

Physical input device → Linux Input Subsystem → Custom Input Handler → Input Ring Buffer → `/dev/vif_driver` → Userspace Recorder → Macro File

**Playback**

Macro File → Userspace Player → `/dev/vif_driver` → Output Ring Buffer → Virtual Input Worker → Virtual Keyboard/Mouse

## Requirements

- Linux with kernel headers installed
- GCC and Make
- Linux `uinput` subsystem
- Root privileges for loading the kernel module and accessing the device

The project is currently developed and tested on Ubuntu running in a Hyper-V-backed Multipass VM.

## Building

Build the kernel module, integration tests, and userspace application:

```bash
./scripts/build_all.sh
```

Build individual components:

```bash
./scripts/build_kernel.sh
./scripts/build_tests.sh
./scripts/build_userspace.sh
```

Clean or rebuild all components:

```bash
./scripts/build_all.sh clean
./scripts/build_all.sh rebuild
```

## Loading the Kernel Module

Load the kernel module with `uinput` support:

```bash
./scripts/load.sh --with-uinput
```

Verify that the device exists:

```bash
ls -l /dev/vif_driver
```

Unload the module:

```bash
./scripts/unload.sh
```

## Recording Macros

Record input events for five seconds:

```bash
sudo ./userspace/vinput record --output demo.vim --duration 5
```

Inspect the recorded macro:

```bash
sudo ./userspace/vinput info demo.vim
```

Replay the macro:

```bash
sudo ./userspace/vinput play demo.vim
```

**Note:** Input entered through SSH or a VS Code Remote terminal is generally not visible to the Linux Input Subsystem. Use a physical or virtual input device exposed to the guest, or generate test events through `uinput`.

## Testing

Build and execute the integration tests:

```bash
./scripts/build_tests.sh
sudo ./tests/run_tests.sh
```

The test suite covers:

- **Device I/O:** Valid and invalid reads, writes, and event structures.
- **Virtual Output:** Keyboard and mouse event injection.
- **Input Capture:** Input handler registration and event delivery.

Tests can also be executed individually:

```bash
sudo ./tests/test_vinput_io
sudo ./tests/test_vinput_output
sudo ./tests/test_vinput_capture
```

## Project Structure

```text
Virtual-Input-Framework/
├── include/
│   └── uapi/
│       └── vinput_uapi.h
├── kernel/
│   ├── vinput_main.c
│   ├── vinput_input.c
│   ├── vinput_ringbuf.c
│   ├── vinput_ringbuf.h
│   ├── vinput_virtual.c
│   └── Makefile
├── userspace/
│   ├── main.c
│   ├── macro.c
│   ├── macro.h
│   ├── recorder.c
│   ├── recorder.h
│   ├── player.c
│   ├── player.h
│   ├── macro_file.c
│   ├── macro_file.h
│   └── Makefile
├── tests/
│   ├── test_vinput_io.c
│   ├── test_vinput_output.c
│   ├── test_vinput_capture.c
│   ├── test_common.c
│   ├── test_common.h
│   ├── run_tests.sh
│   └── Makefile
├── scripts/
│   ├── build_all.sh
│   ├── build_kernel.sh
│   ├── build_tests.sh
│   ├── build_userspace.sh
│   ├── load.sh
│   └── unload.sh
└── README.md
```

## Current Status

- Kernel module builds successfully.
- Userspace macro application builds successfully.
- Integration test binaries compile successfully.
- Kernel device I/O and virtual output have passed initial tests.
- Input capture and end-to-end macro recording/playback require further runtime validation.

## Future Improvements

- More robust handling of dropped input events.
- Device selection and persistent device mapping.
- Macro cancellation and pressed-key cleanup.
- Configurable playback speed and repetition.
- Additional integration and stress tests.

## License

License not yet specified.
