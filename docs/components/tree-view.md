---
title: Tree View
---

<div className="component-page">

# Tree View

<p className="component-lead">Nested content as themed item rows, with a disclosure affordance drawn from the model's own expansion state.</p>

<ComponentPreview name="tree-view" />

## Installation

<ComponentInstall />

## Usage

`TreeView` is a `QTreeView` and reads the same `RowRole`s as
[List View](./list-view.md). Expansion is the model's, so a collapse
survives a repaint and the view never holds a second copy of the state.

```cpp
#include <QStandardItemModel>
#include <shadcn/rows.hpp>

shadcn::TreeView outline(parent);
auto* model = new QStandardItemModel(&outline);
model->setHorizontalHeaderLabels({});
auto* section = new QStandardItem(QStringLiteral("01 Introduction"));
section->appendRow(new QStandardItem(QStringLiteral("Overview")));
model->appendRow(section);
outline.setModel(model);
outline.expand(model->index(0, 0));
```

Clicking the affordance toggles the branch, and the press is consumed there so
the row is not also selected behind the reader's back.

`disclosureRect` gives the affordance's rectangle, which is empty for a row with
no children. It is the hit target, and it is also what a test or a hit check
should aim at.

```cpp
const auto box = outline.disclosureRect(index);
if (box.isValid() && box.contains(position)) { /* the press was on the chevron */ }
```

## API

| Member | Purpose |
| --- | --- |
| `disclosureRect(index)` | The affordance, empty for a leaf. |
| `disclosureSize()` | The affordance's side, in points. |
| `setCompact(bool)` | Sets the row density directly. |
| `setCompactBelow(int)` | The height at which rows become compact. |
| `rowHeight()` | The height one row occupies now. |
| `visibleRowRects()` | The rectangles of the expanded rows on screen. |

## Notes

Qt's own branch indicator is turned off. Left on, a row carries two chevrons:
Qt's at the far left and the themed one beside the text, pointing at each other.

The affordance is placed clear of the row's own fill and border. A one pixel
border running beside a 16 pixel affordance is as tall as the affordance, and it
hides which way the chevron points, which is the one thing the affordance exists
to say.

</div>
