# Working on shadcn-cpp

Version stays at 0.1.0 until the maintainer explicitly requests a change. Never move a published tag. Pin dependency consumers to a commit while this version remains fixed.

Read `docs/spec.md`, the relevant decision, `upstream/manifest.json` and the next issue before editing. Use the highest useful public interface for tests. Add one behaviour, demonstrate its failing test, implement it, then review the result. Keep the change small enough to check against its source.

The upstream commit and profile live in the manifest. Read the exact component wrapper, its examples, theme values and underlying dependency implementation before claiming parity. Record imported dependency revisions too. A wrapper review alone does not establish keyboard or accessibility behaviour.

`draft` means code exists. `implemented` requires a native build and passing interaction tests. `parity-reviewed` also requires named visual, animation and accessibility evidence. Never promote a component because a Qt screenshot resembles itself. Never replace unrun checks with passing placeholders.

Use C++23 and Qt 6.8 or later. Preserve native text editing and button behaviour unless the source requires a documented override. QApplication owns its style. Qt parents own child widgets. Use QPointer for observers that outlive immediate calls. Give lambda connections a receiver context. Keep widget access on the GUI thread. Do not claim Rust-like compile-time lifetime guarantees.

Use `std::expected` for recoverable value errors. Use strong enums, value-owned theme data and constrained helpers where they remove misuse. Avoid speculative backends, allocator frameworks and templates that only shorten implementation code. Measure before changing algorithms for speed.

Keep prose direct, use British spelling and write complete sentences. State the mechanism or measured result. Do not write promotional performance or parity claims. The maintainer's private writing corpus must not be copied into this repository.

Run `python3 tools/check.py`, the core and native tests, sanitizers where supported, the installed consumer and the documentation build. Record unavailable checks in `docs/verification.md`. Run `git diff --check` before committing. Do not publish a release while a required gate is unverified.
