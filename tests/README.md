# Virtual Input Framework integration tests

Place this entire `test/` directory in the **repository root**, alongside `kernel/`
and `include/` (if present). The tests use the real `struct vinput_event` from
`uapi/vinput_uapi.h`. They do not define their own wire-format struct.

## Prerequisites

- Linux guest VM (these tests are **not** run on your Windows host).
- Compiled `kernel/vinput.ko`, based on the updated binary `read()`/`write()` API.
- `uapi/vinput_uapi.h` in `include/uapi/` or `kernel/uapi/`.
- GCC and `make`; root privileges for input device access.
- `uinput` module for the capture test.

## Build and run

From the repository root:

```bash
make -C kernel
sudo insmod kernel/vinput.ko  # Only if not already loaded
sudo modprobe uinput
make -C test
sudo ./test/run_tests.sh
```

If you previously loaded an older module, first close any open device handles
and run `sudo rmmod vinput` before inserting the rebuilt version.

To run only one test:

```bash
sudo ./test/test_vinput_io
sudo ./test/test_vinput_output
sudo ./test/test_vinput_capture
```

If `uapi/vinput_uapi.h` is somewhere else, customize `CPPFLAGS`, for example
`make -C test CPPFLAGS='-D_DEFAULT_SOURCE -I/path/to/parent/of/uapi'`.

The module must already be loaded. The test runner **does not** load or unload
it automatically. Exit code is `0` when all executed tests pass, `1` if any
fail. The capture binary exits `77` when `/dev/uinput` is absent; the runner
reports it as skipped rather than passed.

## What the test programs check

| Test binary | Checks |
| --- | --- |
| `test_vinput_io` | Zero-byte operations; non-record-aligned read/write; unsupported ioctl; nonseekable device; invalid `dev_id`, `reserved`, event type/code/value; accepted SYN record; partial write when a later record is invalid; empty nonblocking read |
| `test_vinput_output` | Valid binary events written through `/dev/vif_driver` arrive at the matching virtual evdev keyboard (F24 down/up) and mouse (REL_X +1/-1), with explicit `SYN_REPORT` boundaries |
| `test_vinput_capture` | A temporary `uinput` device produces F24 events that the module captures and returns via `read()`; checks timestamp, `reserved`, source ID consistency, and order of two synchronization frames |

**Important:** These are live kernel integration tests. They produce an F24 key
press/release and two tiny mouse movements in the guest. Run them in a test VM,
not your daily desktop or host. Keep the VM input quiet during capture tests.

When a test fails, inspect:

```bash
sudo dmesg | tail -60
ls -l /dev/vif_driver /dev/uinput
cat /proc/bus/input/devices | grep -A 5 'vinput virtual'
```

## Coverage limits

- These tests do **not** exercise macro playback, ring-buffer concurrency under
  load, long-term device churn, or kernel memory checking.
- Empty nonblocking reads depend on there being no simultaneous physical input;
  the check is skipped if the capture stream will not drain.
- Failed `write()` calls are tested at the syscall boundary. No test checks that
  malformed events can never alter kernel state under adversarial concurrency.
- If a source change modifies the driver’s ABI or supported output-event rules,
  update the tests together with it.
