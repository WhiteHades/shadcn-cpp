# Architecture

`shadcn::core` contains theme values, validation and timing functions. It has no Qt dependency and no platform state. `shadcn::widgets` links the core to Qt Widgets. The gallery and applications consume the widget API rather than internal paint helpers.

Qt handles application windows, input dispatch, text editing, parent ownership, layout and the base accessibility interfaces. Component subclasses customise drawing and add the source-specific state they need. Shared paint and timing helpers stay private. There is one native implementation, so there is no speculative renderer interface or backend plugin system.

`Style` owns a theme copy. QApplication owns the installed Style. Components read the current theme through their style and update after style changes. The initial API uses application-wide installation. Nested themes and additional style profiles need their own design and tests before becoming public configuration.

Variant and size enums avoid stringly typed component options. The constrained `make_child` helper returns a parent-owned widget by borrowed reference. It keeps ordinary Qt layout and signal APIs available. A JSX-like builder would add another object-lifetime and composition model without helping the initial port.

The manifest separates source identity, implementation status and review evidence. The catalogue records the planned scope. The issue files divide the work into small increments with dependencies. This follows the reviewed planning guidance through a spec, testable public interfaces and evidence-backed implementation steps; it does not imply that the whole library is complete.

The decisions directory records the current backend, source profile and documentation choices. Change those decisions explicitly when a new requirement requires it. Keep the runtime C++ library independent of the documentation build tools.
