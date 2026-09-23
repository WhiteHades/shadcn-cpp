<h1 align="center">shadcn-cpp</h1>
<p align="center">shadcn/ui components for native C++ applications.</p>
<p align="center">
  <a href="LICENSE"><img alt="MIT licence" src="https://img.shields.io/badge/licence-MIT-84cc16?style=flat-square" /></a>
  <img alt="CMake 3.25 or later" src="https://img.shields.io/badge/CMake-3.25%2B-2563eb?logo=cmake&amp;style=flat-square" />
  <img alt="C++23 and C++26" src="https://img.shields.io/badge/C%2B%2B-23%20%2F%2026-2563eb?logo=cplusplus&amp;style=flat-square" />
  <img alt="Qt 6.8 or later" src="https://img.shields.io/badge/Qt-6.8%2B-41cd52?logo=qt&amp;style=flat-square" />
</p>
<p align="center">
  <img alt="Linux tested" src="https://img.shields.io/badge/Linux-tested-22c55e?logo=linux&amp;style=flat-square" />
  <img alt="Windows verification pending" src="https://img.shields.io/badge/Windows-pending-737373?style=flat-square" />
  <img alt="macOS verification pending" src="https://img.shields.io/badge/macOS-pending-737373?logo=apple&amp;style=flat-square" />
</p>
<p align="center"><a href="https://whitehades.github.io/shadcn-cpp/">Documentation</a> · <a href="https://whitehades.github.io/shadcn-cpp/components">Components</a> · <a href="https://whitehades.github.io/shadcn-cpp/api">C++ API</a></p>

The design and motion of shadcn, built with native C++ and Qt Widgets. Small APIs, shared themes and ordinary Qt ownership. Builds use C++26 where supported, with C++23 as the baseline.

<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="assets/previews/card-dark.png" />
    <img src="assets/previews/card-light.png" alt="Native card with a project name field and Cancel and Create buttons" width="640" />
  </picture>
</p>

Version **0.1.0** is in development. Linux builds and tests pass with Qt 6.11.2. Windows and macOS verification is pending.

## Build

Install a C++23 compiler, CMake 3.25 or later, Ninja and Qt 6.8 or later with Widgets.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/shadcn_gallery
```

## Use

```cpp
#include <shadcn/shadcn.hpp>

shadcn::install(app);

auto& button = shadcn::make_child<shadcn::Button>(window, "Continue");
button.setVariant(shadcn::Variant::Outline);
layout.addWidget(&button);
```

Link `shadcn::widgets`. Qt parents own their child widgets. The [installation guide](https://whitehades.github.io/shadcn-cpp/getting-started) includes a complete application and CMake example. `shadcn::core` is also available without Qt.

MIT. Adapted from [shadcn/ui](https://github.com/shadcn-ui/ui), with its [licence retained](LICENSES/shadcn-MIT.txt). This is an independent port.
