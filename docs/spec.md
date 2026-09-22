# Native shadcn port specification

## Problem

A C++ desktop application needs shadcn's component appearance and behaviour without a browser runtime. A theme alone cannot supply menus, focus behaviour, animation, accessibility or application composition. The port needs an explicit source reference and evidence that each native component implements its advertised scope.

## Intended result

Applications link a native C++23 library, choose a reviewed theme/profile, create components with a small typed API and connect ordinary Qt signals. Components support the source's variants, sizes and states. Guides, a gallery and a generated C++ reference explain integration. The complete scope includes all upstream component categories, related recipes, design profiles and useful native distribution tooling, delivered in increments.

Version remains 0.1.0 until the maintainer explicitly authorises a change. Every development snapshot has a source commit identity. No moving release tags are allowed.

## Application requirements

1. A developer can add a pinned source snapshot or find an installed CMake package.
2. A developer can use an individual component without constructing an unrelated application framework.
3. An application can use shared neutral colours, typography, radii and spacing, then customise documented theme values.
4. A learner can activate buttons with a pointer or keyboard, identify disabled and invalid states, and see a visible keyboard focus indicator.
5. A text field retains selection, clipboard, undo, password and input-method behaviour.
6. A label exposes an accessible relationship to its control and supports the matching activation behaviour.
7. A user can select checked, unchecked and mixed states wherever the source allows them.
8. A user can turn off motion. Hidden components do not keep unnecessary animations running.
9. A user can open and dismiss nested menus and dialogs without losing focus or interacting with blocked content.
10. An application can place popups near screen edges and handle multiple screens and display scales.
11. A user can navigate tabs, groups and lists with the source's keyboard rules and right-to-left adaptations.
12. A user can use comboboxes, command search, calendars, date ranges and validated forms without missing input or cancellation behaviour.
13. A developer can display large data sets through model/view components with documented sorting, filtering, selection and virtualisation costs.
14. A user can understand progress, alerts, notifications, empty states and loading placeholders through visual and assistive interfaces.
15. A developer can compose sidebars, cards, learning screens and upstream block examples from the same component API.
16. A maintainer can trace a component to an exact upstream wrapper and the dependencies that supply its behaviour.
17. A maintainer can discover upstream changes without silently changing the port's reference.
18. A contributor can reproduce build, interaction, visual, animation and accessibility checks.
19. A user can read installation, ownership and component documentation without a large framework tutorial.
20. A maintainer can publish a reviewed snapshot with licence notices and an accurate support matrix.

## Implementation decisions

The first backend is Qt Widgets. The runtime contains C++ and links Qt. The first source profile is New York v4 and its neutral theme. Additional profiles are separate scope, not assumed compatible because they share component names.

A Qt-free module owns design values, conversion and timing. Native widgets reuse Qt behaviour when its contract matches the component. Source-specific overrides remain small and tested. Reusable application compositions follow after the components they require.

The public interface includes ownership, thread-affinity, error and performance rules. It uses value types, strong enums, constrained helpers and expected results where those mechanisms prevent a concrete mistake. There is no blanket Rust-equivalent memory-safety or best-possible-performance claim.

The source manifest records hashes and independent evidence categories. A draft can exist without native verification, but it cannot be labelled complete. Wrapper review does not substitute for reading a Radix or Base UI implementation when it owns the behaviour.

Documentation uses Docusaurus for the guides and a C++ API reference generated from declarations and comments with clang-doc. The documentation build tools are not application runtime dependencies.

## Testing decisions

Test the public core and native component interfaces. Use literal reference values or independently calculated examples for expected results. Cover invalid values and interrupted interactions. Keep object-lifetime tests under sanitizers on supported toolchains.

Visual review requires a browser implementation of the pinned source and a native implementation with matched fonts, scale, states and assets. Animation review includes intermediate frames and cancellation. Real desktop and assistive-technology sessions remain separate gates.

An installed-consumer test catches packaging errors. Documentation compilation and link checks catch stale guides. Platform CI is required before the platform is listed as supported.

## Current increment

The first increment writes the core and ten draft controls. It includes tests, a gallery, source records, planning files, CI configuration and documentation source. The current verification report is the source of truth for what actually ran. No native component has earned a parity-reviewed status yet.

## Later scope

All remaining catalogue entries, alternate profiles, theme presets, external behaviour dependencies, recipes, blocks and suitable native equivalents remain in the roadmap. Browser-only mechanisms need documented native mappings. The goal does not require cloning React, Tailwind or a browser DOM as a second runtime.

A course player's decoding, local file indexing and course-progress database belong in an application that uses this library. They are not component-library responsibilities.
