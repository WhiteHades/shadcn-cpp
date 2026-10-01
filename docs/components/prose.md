---
title: Prose
---

<div className="component-page">

# Prose

<p className="component-lead">A read-only, theme-coloured surface for a document body.</p>

<ComponentPreview name="prose" />

## Installation

<ComponentInstall />

## Usage

A lesson is read, not typed into. `Prose` is a `QTextEdit` under the theme's
typography: read only, selectable by mouse and by keyboard, focusable, and with no
caret.

```cpp
#include <shadcn/rows.hpp>

shadcn::Prose prose(parent);
prose.setHtml(QStringLiteral(
    "<h1>Lesson 4</h1>"
    "<p>A system takes an input and produces an output.</p>"
    "<ul><li>What an input is</li><li>Why the middle is the hard part</li></ul>"
    "<p>Read <a href=\"#\">the notes for lesson 3</a> first.</p>"));
```

The document structure is available to a caller that needs it rather than the
text, which is what a table of contents or a progress indicator is built from.

```cpp
prose.headings();       // { "Lesson 4", ... }
prose.headingLevels();  // { 1, 2, ... }
```

## API

| Member | Purpose |
| --- | --- |
| `applyTypography()` | Rebuilds the document stylesheet from the theme. Called on a theme or font change. |
| `headings()` | The text of each heading block, in order. |
| `headingLevels()` | The level of each heading block, in order. |
| `readerMode()` | True when the surface cannot be typed into. |
| `setReaderMode(bool)` | Turns editing on or off, keeping selection either way. |

## Notes

The text layout is Qt's. A document engine is not this library's work, and
reimplementing wrapping would lose selection, scrolling, and the accessibility a
real text surface provides for free. The type scale and the colours are the
theme's, and a theme change rebuilds the stylesheet without touching the document,
so the text, the selection and the scroll position all survive.

The surface is the page, not a field. A text edit's own base colour is the
platform's idea of a field, which is the one surface a reader never asked for.

List items get padding, which is the one property that moves Qt's list marker left
and its text right together. Margin and text-indent move both the same way,
leaving the gap at nothing, so a list reads as one run.

</div>
