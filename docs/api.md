---
slug: /api
sidebar_position: 3
---

# C++ API

The reference is generated from [`include/shadcn/core.hpp`](https://github.com/WhiteHades/shadcn-cpp/blob/main/include/shadcn/core.hpp) and [`include/shadcn/widgets.hpp`](https://github.com/WhiteHades/shadcn-cpp/blob/main/include/shadcn/widgets.hpp) with `clang-doc`. It includes the declarations as clang-doc renders them and the comments that ship in the public headers. Open **Generated API** in the sidebar for the namespace, class, enum and function pages.

The site build configures a small CMake compile database with Clang, runs `clang-doc --format=md --public`, and passes the resulting Markdown to Docusaurus. The generated pages are build output and are not checked into the repository. Run `bash tools/generate-api.sh` when you want to inspect them locally.

The Markdown formatter can omit default arguments and qualifiers. The linked headers remain the exact declaration source. Qt's macro-generated metadata members are removed from the generated pages so the reference focuses on the library's own API.

## Targets and includes

The library uses C++23. Include `shadcn/core.hpp` for the Qt-free value and timing API. Include `shadcn/widgets.hpp` for the Qt Widgets classes. The widget target requires Qt 6.8 or later.

```cmake
find_package(shadcn 0.1.0 EXACT REQUIRED COMPONENTS core widgets)
target_link_libraries(app PRIVATE shadcn::core shadcn::widgets)
```

```cpp
#include <shadcn/core.hpp>    // no Qt dependency
#include <shadcn/widgets.hpp> // Qt Widgets API
```

All names are in the `shadcn` namespace. The version constant is `shadcn::version`, currently `"0.1.0"`.

## Ownership and threads

Qt owns a widget constructed with a parent. `make_child` returns a borrowed reference to that widget:

```cpp
auto& button = shadcn::make_child<shadcn::Button>(window, "Continue");
layout.addWidget(&button);
```

Do not delete the returned reference or put it in another owning smart pointer. Do not retain it after its parent is destroyed. Use `QPointer<T>` when an observer may outlive the immediate call. Create and access widgets on the GUI thread. Ordinary Qt signals, layouts and inherited widget methods remain available.

## Application setup

`Style` owns a `Theme` copy and a `MotionPolicy`. `install(QApplication&, Theme, MotionPolicy, int fontPixels)` installs it on the application. A non-positive `fontPixels` keeps the application's current font size. `QApplication` owns the installed style.

```cpp
QApplication app(argc, argv);
shadcn::install(app, shadcn::Theme::neutral(), shadcn::MotionPolicy::Reduced);
```

`make_child<T>(QWidget&, Args&&...)` constructs a `QWidget`-derived type whose constructor accepts the forwarded arguments followed by a `QWidget*` parent. It returns `T&` and transfers ownership to the parent.

## Composing a card

The generated `Card` reference lists its members. Its layouts are borrowed references to card-owned hosts, so child widgets can use ordinary Qt composition:

```cpp
auto& card = shadcn::make_child<shadcn::Card>(window);
card.setTitle("Modern C++");
card.setDescription("Continue the current lesson.");
auto& button = shadcn::make_child<shadcn::Button>(card, "Continue");
card.content().addWidget(&button);
```

## Source and support status

The API follows the pinned New York v4, neutral profile at commit [`98a1fe6`](https://github.com/shadcn-ui/ui/tree/98a1fe67b439324ddc857f47fbdce056600a4329). The ten widget classes remain drafts. The Linux native build and tests pass, while cross-platform, visual, animation and accessibility parity checks remain pending. Component-specific scope and known differences are listed in the [component catalogue](components/index.md).
