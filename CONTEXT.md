# Project context

shadcn-cpp is an independent native C++ port of shadcn/ui. The first profile is New York v4 with the neutral theme. Qt Widgets handles windows, input and accessibility; C++ code handles component rendering and shared design values.

A component is a reusable widget or composition. A profile is a particular upstream style and theme combination. A source pin identifies an immutable upstream commit. Parity means agreement with a named source profile across appearance, interaction, animation and accessibility, with documented native differences. It never means that TSX executes in C++.

The current delivery has a tested Qt-free core and draft native widgets. The Qt build, browser comparison, documentation build and GitHub publication remain unverified or unavailable. See `docs/verification.md` for the exact run results.

The tracker is the numbered Markdown issue directory under `docs/issues`. Each issue records dependencies, acceptance criteria and evidence. These are local planning files, not already-created GitHub issues. Version remains 0.1.0.
