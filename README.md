<h1 align="center">shadcn-cpp</h1>
<p align="center">Native C++23 and Qt 6 UI components inspired by shadcn/ui.</p>
<p align="center">
  <img alt="C++23" src="https://img.shields.io/badge/C%2B%2B-23-18181b?style=flat-square" />
  <img alt="Qt 6.8+" src="https://img.shields.io/badge/Qt-6.8%2B-18181b?style=flat-square" />
  <img alt="Version 0.1.0" src="https://img.shields.io/badge/version-0.1.0-18181b?style=flat-square" />
  <img alt="MIT" src="https://img.shields.io/badge/licence-MIT-18181b?style=flat-square" />
</p>
<p align="center"><a href="https://whitehades.github.io/shadcn-cpp/">Documentation</a> · <a href="docs/getting-started.md">Get started</a> · <a href="docs/components/index.md">Components</a> · <a href="docs/roadmap.md">Roadmap</a> · <a href="docs/verification.md">Verification</a></p>

**Early development.** The Qt-free core and ten draft native components build and pass their tests on Linux with Qt 6.11.2. Cross-platform verification, visual parity and platform accessibility checks are still pending. This is not a complete shadcn port.

The first reference is shadcn's New York v4 profile and neutral theme at [`98a1fe6`](https://github.com/shadcn-ui/ui/tree/98a1fe67b439324ddc857f47fbdce056600a4329). Each draft records its source hash and known differences in the [parity ledger](upstream/manifest.json). The application runtime uses C++ and Qt Widgets.

## Build the gallery

Install a C++23 compiler, CMake 3.25 or later, Ninja and Qt 6.8 or later with Widgets and Test.

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
./build/dev/shadcn_gallery
```

## Use a component

```cpp
shadcn::install(app);

auto& button = shadcn::make_child<shadcn::Button>(window, "Continue lesson");
button.setVariant(shadcn::Variant::Outline);
layout.addWidget(&button);
```

Link `shadcn::widgets`. The [complete example](docs/getting-started.md) includes CMake, headers and ownership rules. `shadcn::core` is available separately without Qt.

## Status

Drafts cover Button, Card, Input, Badge, Label, Checkbox, Switch, Separator, Progress and Skeleton. The [roadmap](docs/roadmap.md) covers the remaining components, recipes and design profiles. Docusaurus builds the guides and [C++ API reference](docs/api.md) as one documentation site.

Version remains **0.1.0** until the maintainer requests a change. Use commit pins during development.

MIT. Includes adaptations of [shadcn/ui](https://github.com/shadcn-ui/ui), with its [licence retained](LICENSES/shadcn-MIT.txt). Qt has its own licence. This project is independent of shadcn/ui.
