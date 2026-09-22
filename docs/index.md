---
slug: /
sidebar_position: 1
---
# shadcn-cpp

A native C++ port of shadcn/ui. The project starts with Qt Widgets and a pinned New York v4 reference, then adds components through source review and behaviour tests.

The current snapshot contains a tested Qt-free core and ten draft widgets. The Linux native build and tests pass with Qt 6.11.2. Cross-platform verification and visual parity remain pending. Read the [verification report](verification.md) before integrating the widgets into an application.

Start with [installation and the complete example](getting-started.md). The [component catalogue](components/index.md) separates draft implementations from planned work. The [roadmap](roadmap.md) gives the order of the next increments.

Version stays at 0.1.0 until the maintainer requests a change. Source commits identify development snapshots. No stable ABI or complete shadcn compatibility is promised for this snapshot.
