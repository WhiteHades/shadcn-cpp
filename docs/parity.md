# Porting and parity

The manifest pins one upstream commit and records a Git blob hash for every reviewed wrapper. This makes the source reference repeatable. It does not establish that the native implementation matches it.

The initial profile is `apps/v4/registry/new-york-v4/ui` with the neutral theme. The source repository also contains other profiles, examples, compositions, hooks and command-line tooling. Some behaviours come from external packages such as Radix, Base UI, cmdk, Vaul and chart libraries. Read and pin the relevant dependency implementation before declaring that behaviour ported.

## Check the source

Use a separate checkout of the upstream repository. The audit tool only reads Git objects and never runs upstream scripts.

```sh
git clone https://github.com/shadcn-ui/ui.git ../shadcn-ui
python3 tools/upstream.py verify --checkout ../shadcn-ui
python3 tools/upstream.py inventory --checkout ../shadcn-ui
```

The verifier checks the exact objects recorded in the manifest. The inventory enumerates TSX files in the selected profile, including support files that may not be separate registry entries. A fixture-based local test verifies the tool, but the complete upstream checkout check has not run in the authoring environment.

To inspect a newer upstream revision, fetch it in that checkout, obtain its full SHA, then run:

```sh
python3 tools/upstream.py changes --checkout ../shadcn-ui --candidate FULL_40_CHARACTER_SHA
```

The tool prints changed paths. It does not overwrite the source pin, change a component's status or bump the project version. The optional upstream workflow performs source-object checks on demand.

## Implement one component

Read the wrapper, examples, theme values and behaviour dependency. Record variants, sizes, states, focus rules, keyboard actions, dismissal behaviour, reduced motion and accessibility information. Write the public-interface test for one behaviour, implement it, then repeat. Keep native adaptations explicit.

A component record moves through these states:

| State | Required evidence |
| --- | --- |
| planned | Catalogue entry only. |
| draft | Source review and C++ implementation exist. Native checks may still be missing. |
| implemented | Native build and interaction tests pass, with evidence files. |
| parity-reviewed | Visual, animation and accessibility reviews also pass, with named evidence and differences. |

The manifest checker rejects a promotion without the required pass fields and evidence files. It verifies record consistency, not the truth or quality of a screenshot review. Human review still has to inspect that evidence.

## Compare appearance and motion

Render the exact pinned browser component and its Qt counterpart with the same font, dimensions, theme, scale, text, icons and state. Capture light and dark themes at 100%, 125%, 150% and 200% display scale. Include pointer hover, keyboard focus, pressed, checked, mixed, disabled, invalid and loading states where applicable.

Record the operating system, browser version, Qt version, display backend and font identity with each capture. Compare geometry, colour, typography, borders, rings, shadows and clipping. Record region-level differences and inspect the images; do not approve a whole window from a single loose pixel threshold.

For animation, compare start and end states plus intermediate frames and interrupted transitions. Check reduced motion, hidden widgets and rapid state reversals. A screenshot of a Qt widget compared with an earlier Qt screenshot is a regression check, not evidence of shadcn parity.

The current gallery can capture a Qt PNG. No browser-reference renderer or approved baseline set is included yet. Building those comparisons is the next visual-verification issue.

## Platform behaviour

Keyboard and pointer tests must cover normal activation and cancellation, focus restoration, nested popups, scrolling and window-edge placement as components gain those behaviours. Run real screen-reader and input-method checks separately from headless tests. Include mixed Arabic and Latin text, right-to-left layouts and large text.

Do not silently replace a missing popup, date picker or data-table behaviour with a stock Qt control and mark the port complete. A native substitution belongs in the documented differences until it satisfies the chosen reference contract.
