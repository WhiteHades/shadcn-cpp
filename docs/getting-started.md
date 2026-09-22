# Get started

## Requirements

Use a C++23 compiler and standard library with `std::expected`, CMake 3.25 or later and Ninja. Native widgets also require Qt 6.8 or later with Widgets. Tests require Qt Test. The Qt-free core has been checked with GCC 14.2 in this environment; the native platform matrix is configured but has not run.

The audit scripts require Python 3.10 or later. The native library does not depend on Python, Node.js or a browser.

Qt must be discoverable through CMake. With a separate Qt SDK, pass its installation directory through `CMAKE_PREFIX_PATH`. Keep that machine-specific path in your shell or `CMakeUserPresets.json`.

## Build the native gallery

Run from the repository root:

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
./build/dev/shadcn_gallery
```

On a headless Linux machine, the tests select Qt's offscreen platform. A real desktop session is still required for window-manager, input-method and screen-reader checks. The current authoring environment has no Qt SDK, so these native commands have not completed here.

## Add the source to your application

Place a reviewed source snapshot at `third_party/shadcn-cpp`. This avoids any network download during configuration.

```cmake
cmake_minimum_required(VERSION 3.25)
project(course_viewer LANGUAGES CXX)
set(SHADCN_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SHADCN_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/shadcn-cpp)
add_executable(course_viewer main.cpp)
target_link_libraries(course_viewer PRIVATE shadcn::widgets)
```

The public widget API uses Qt types so that ordinary layouts, signals and application code work together. The component pages define the supported scope. Inherited Qt appearance options are not all implemented by the custom painters. This complete example creates a card and a button:

```cpp
#include <shadcn/widgets.hpp>
#include <QApplication>
#include <QVBoxLayout>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    shadcn::install(app);

    QWidget window;
    QVBoxLayout layout(&window);
    auto& card = shadcn::make_child<shadcn::Card>(window);
    card.setTitle("Modern C++");
    card.setDescription("Continue the current lesson.");
    layout.addWidget(&card);

    auto& button = shadcn::make_child<shadcn::Button>(card, "Continue lesson");
    button.setVariant(shadcn::Variant::Outline);
    card.content().addWidget(&button);
    QObject::connect(&button, &QPushButton::clicked, &window, [&window] {
        window.setWindowTitle("Lesson opened");
    });

    window.resize(540, 280);
    window.show();
    return app.exec();
}
```

`make_child` gives the parent ownership and returns a borrowed reference. Do not delete the reference or keep it after the parent has been destroyed. Use `QPointer` for a delayed observer. See [ownership and safety](safety.md).

## Install a CMake package

```sh
cmake --install build/dev --prefix "$HOME/.local"
```

An application can then call `find_package(shadcn 0.1.0 EXACT REQUIRED COMPONENTS widgets)` and link `shadcn::widgets`. Its compiler, standard library and Qt kit must match the installed package. This snapshot does not publish binary packages or promise ABI compatibility.

To build only the Qt-free core:

```sh
cmake --preset core
cmake --build --preset core
ctest --preset core
cmake --install build/core --prefix "$HOME/.local"
```

That installation exposes `shadcn::core` without Qt. A package built with widgets includes the Qt dependency in its package configuration. The installed-consumer example exercises the core-only package.

## Pin a development snapshot

After publication, use an exact repository commit with CMake FetchContent, or vendor that commit. Do not use a moving branch for a production build. The project version remains 0.1.0, so a version string alone does not identify the source snapshot.
