# Contributing

Read the [current status](docs/verification.md), [porting process](docs/parity.md) and [next issue](docs/issues/01-native-verification.md). Work on one component or one shared behaviour at a time.

Record the upstream commit, source file and blob hash before changing a component. Read its dependencies when they define the behaviour being changed. Add a public-interface regression test, then implement the change. Keep screenshots, interaction results and known differences beside the component record.

Use the developer preset to build the native library and gallery. The core preset does not build any UI.

```sh
python3 tools/check.py
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

The [testing guide](docs/testing.md) covers sanitizers, installed consumers and visual review. Add API comments to public declarations. Keep the README short and put detailed examples in the documentation.

Version stays at 0.1.0. Do not add an automatic version bump, move a release tag or claim stable ABI compatibility. Open an ordinary pull request with the source reference, behaviour changed, checks run and remaining limitations.
