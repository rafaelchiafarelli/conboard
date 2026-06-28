# conboard unit tests

Pure-logic tests with **synthetic inputs** — no real hardware, no `/dev/input`.
They run identically on the host (fast) and under QEMU (arch-correctness).

Framework: [doctest](https://github.com/doctest/doctest) — a single vendored
header (`doctest.h`), like the project's other vendored headers.

## Run

```bash
./run-tests.sh                        # all tests, natively (needs host g++/cmake)
./run-tests.sh joystick               # only the "joystick" suite
./run-tests.sh joystick:hitTheAButton # only that case
./run-tests.sh --qemu zero3           # all tests under emulation for zero3's arch
./run-tests.sh --qemu zero3 joystick  # filtered, under emulation
```

`ctest -R joystick` also works (each suite is registered as a CTest test).

## Adding tests

Tests target **pure functions** fed synthetic data. The first suite covers the
device classifier (`condetect::classifyEvdev`) — e.g. "a gamepad reporting
`BTN_A` + abs axes classifies as a joystick". To make a module testable, extract
its decision logic into a pure function (as was done for the classifier) and add
a `test_*.cpp` plus its source to `tests/CMakeLists.txt`.

Anything that needs real hardware (a physical button press producing an event)
is out of scope here — use `tools/devprobe` on the board for that.
