# Add the themed row views and the prose surface

Status: implemented.

## Dependencies

06-data-navigation.

## Scope

Add the pieces an application needs to build a whole screen out of components,
rather than hand-painting its own rows.

- `ListView`: a virtualised list that paints shadcn `Item` rows, with the accent
  fill on selection, the muted fill on hover, the ring on keyboard focus, and
  keyboard movement. It is a `QListView`, so a large set costs only the visible
  rows.
- `TreeView`: the same rows for nested content, with a disclosure affordance and
  indentation, because a course outline is a tree and a one-widget-per-node
  accordion is not usable at a hundred thousand lessons.
- `Prose`: a read-only rich text surface for a document body, so a lesson is not
  read in a stock `QTextEdit`.

A `StatusBar` was in the original scope and is not in this change. A status line
is a row of a `RowDelegate` and a `Layout`, and a component that only arranges
other components earns its place later, if at all.

## Why not a model and a view over the existing Item

`Item` is one widget. One widget per row is right for a page of items and wrong
for a list that pages through an unbounded set, and the library's own rule for
data views is to avoid one widget per row. So the views read the standard Qt item
data roles and paint the same geometry `Item` paints, which keeps one description
of a row rather than two.

## Decisions

- Row content comes from `Qt::DisplayRole` for the title, a `RowRole` for the
  description, and leading and trailing roles for content either side. A caller
  supplies data rather than widgets. Anything a caller cannot express in those
  roles is not drawn, rather than drawn differently.
- The accent fill carries the accent foreground. A selected row whose muted text
  stayed the page colour would fail contrast on the fill, and that is the common
  way a themed list goes wrong.
- Every row is the same height, whether or not it has a description; a
  description occupies the second line whether or not it has text. This is what
  lets the view build only the rows on screen. A view whose rows differ in height
  has to measure all of them to find where the next one starts, and the suite
  caught exactly that: with a per-row height, 400 rows were read for a 160 pixel
  list.
- Selection is not signalled by colour alone: the focused row is outlined, and a
  selected row reports itself to assistive technology through the view's own
  selection model rather than through a per-row widget.
- The disclosure affordance is drawn, not a button, and the expand state comes
  from the model rather than a parallel flag. Clicking it toggles the model.
- The affordance is placed clear of the row's own fill and border. A one pixel
  border running beside a 16 pixel affordance is as tall as the affordance, and it
  hides which way the chevron points, which is the one thing the affordance
  exists to say.
- Qt's own branch indicator is turned off. Left on, a row carries two chevrons:
  Qt's at the far left and the themed one beside the text, pointing at each other.
- The density breakpoint lives in the view rather than in the caller, because a
  breakpoint both hold is a breakpoint that disagrees with itself.
- Reduced motion keeps every state reachable. The only animation is the fill and
  ring cross-fade, and it is dropped rather than shortened.
- `Prose` keeps Qt's text layout and takes only the type scale and the colours
  from the theme. A document engine is not this library's work, and reimplementing
  wrapping would lose selection, find, and the accessibility a real text surface
  provides for free.
- `Prose` is a reader, not a form: read only, selectable by mouse and by keyboard,
  focusable, and with no caret. A reader still has to be able to copy from a
  lesson, so reader mode drops `TextEditable` and keeps selection.
- List items get `padding-left`, which is the one property that moves Qt's list
  marker left and its text right together. Margin and text-indent move both the
  same way, leaving the gap at nothing, so a list reads as one run.

## Acceptance

Met, and each one is a test in `tests/rows_test.cpp`:

- Rows paint from theme roles in both colour modes, and a selected row's text
  clears 4.5:1 on its own fill. Measured over the whole selected row, so the
  check reads the text and not the fill it sits on.
- A thousand rows cost the visible rows, proven by counting what the view asks the
  model for rather than by asserting it. 400 rows, a 160 pixel list, under 64
  reads.
- Arrow keys, Home, End and Page keys move and select, and the view keeps Qt's
  selection model so assistive technology and `QItemSelectionModel` work.
- A theme change is a repaint, not a data rebuild: a 2000 row model is not walked
  when the theme moves.
- The density switch follows the available height, the compact row is shorter, and
  a compact row still clears 24 pixels.
- The disclosure affordance matches the model's expansion in both directions,
  clicking it toggles the model, and the two states are told apart by the shape's
  own extent rather than by a pixel count, which a rotation does not change.
- `Prose` keeps the document selectable and scrollable, exposes its heading
  structure, and its surface is the page rather than a field.
- Motion policy does not change row geometry, so reduced motion is never the only
  way a state is reached.

## Bugs found and fixed while building

- A per-row height defeated the view's virtualisation. Recorded above.
- Applying a stylesheet from inside the style-change handler recursed until the
  stack ran out. The scrollbar sheet is now written at most once per theme, in
  `src/scrollbar_style.hpp`, shared with `ScrollArea`.
- Qt's branch indicator drew a second chevron on every row.
- The chevron was small enough inside its 16 pixel box to be a smudge.
- List markers sat hard against their text.
- `ScrollArea` and `Prose` would have drifted apart on scrollbar chrome, so the
  rule moved into one internal header.

## Evidence

`tests/rows_test.cpp`, eleven interaction cases, all passing in `build`.
`ctest -E media` passes 8 of 8. `tools/check.py`, `tests/check_test.py` and
`tests/api_test.mjs` pass.

Gallery entries `list-view`, `tree-view` and `prose`, reviewed as rendered
captures in both colour modes.
