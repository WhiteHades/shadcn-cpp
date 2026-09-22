# Testing

Tests call the same public interfaces as applications. The Qt-free suite covers colour conversion, theme validation, progress ranges and timing functions. The native test file covers activation, editing, buddy labels, progress state, accessible interfaces, parent lifetime and a rendering smoke test.

## Core and sanitizers

```sh
cmake --preset core
cmake --build --preset core
ctest --preset core
cmake --preset core-asan
cmake --build --preset core-asan
ctest --preset core-asan
python3 -m unittest discover -s tests -p '*_test.py' -v
python3 tools/check.py
```

The core executable currently runs 1043 assertions, including a 1001-point monotonicity sweep. CTest registers it as one test executable. This count is not 1043 independent features or a coverage percentage.

## Native checks

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
cmake --preset asan
cmake --build --preset asan
ctest --preset asan
```

The sanitizer presets support GCC and Clang with AddressSanitizer and UndefinedBehaviorSanitizer. They require the corresponding runtime libraries. Native checks have not run in the authoring environment. The three-platform workflow is configuration awaiting its first run.

## Installed consumer

```sh
cmake --install build/core --prefix /tmp/shadcn-install
cmake -S examples/consumer -B build/consumer -G Ninja -DCMAKE_PREFIX_PATH=/tmp/shadcn-install
cmake --build build/consumer
ctest --test-dir build/consumer --output-on-failure
```

A source-tree build can pass while an installed package is broken. This test checks an application using `find_package` and the installed headers and archive.

## Rendering and accessibility

```sh
./build/dev/shadcn_gallery --dark --reduced-motion --screenshot screenshots/gallery-dark.png
./build/dev/shadcn_gallery --rtl --reduced-motion
```

The capture command is a diagnostic convenience. See [the parity process](parity.md) for the independent browser references still required. Native QAccessible checks do not replace VoiceOver, NVDA or Orca sessions, and offscreen tests do not exercise a real compositor or input method.

For every failed or unavailable gate, keep its command and error in the verification report. A configured CI workflow is not a passing CI run.
