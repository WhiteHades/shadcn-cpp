---
title: List View
---

<div className="component-page">

# List View

<p className="component-lead">A virtualised list of themed item rows, for a set too large to be one widget per row.</p>

<ComponentPreview name="list-view" />

## Installation

<ComponentInstall />

## Usage

`ListView` is a `QListView`, so a caller fills a model and the rows on screen are
the only rows built. The title comes from `Qt::DisplayRole`; the rest of a row
comes from a `RowRole`.

```cpp
#include <QStandardItemModel>
#include <shadcn/rows.hpp>

shadcn::ListView list(parent);
auto* model = new QStandardItemModel(&list);
auto* item = new QStandardItem(QStringLiteral("Systems 101"));
item->setData(QStringLiteral("Lesson 4 of 12"),
              static_cast<int>(shadcn::RowRole::Description));
item->setData(QStringLiteral("41%"),
              static_cast<int>(shadcn::RowRole::TrailingText));
model->appendRow(item);
list.setModel(model);
list.setCurrentIndex(model->index(0, 0));
```

Set `RowRole::Heading` on a row to make it a label for the rows under it rather
than a target: it takes no fill, no ring and no selection.

```cpp
auto* heading = new QStandardItem(QStringLiteral("In progress"));
heading->setData(true, static_cast<int>(shadcn::RowRole::Heading));
heading->setFlags(Qt::ItemIsEnabled);
```

`setCompactBelow` gives the rows a density switch the caller does not have to
own: below the given height the rows become shorter, and a compact row is still
the same row.

```cpp
list.setCompactBelow(400);
```

## API

| Member | Purpose |
| --- | --- |
| `setCompact(bool)` | Sets the row density directly. |
| `compactRows()` | The current density. |
| `setCompactBelow(int)` | The height at which rows become compact. Zero disables the switch. |
| `compactBelow()` | That height. |
| `rowHeight()` | The height one row occupies now. |
| `visibleRowRects()` | The rectangles of the rows on screen. |

### RowRole

| Role | Carries |
| --- | --- |
| `Qt::DisplayRole` | The title. |
| `Description` | The muted line under the title. |
| `Leading` | A `QPixmap` before the text. |
| `Trailing` | A `QPixmap` after the text. |
| `TrailingText` | A short muted label on the trailing side. |
| `Heading` | Marks a row as a label rather than a target. |

## Notes

Every row is the same height, so a description occupies the second line whether
or not it has text. A view whose rows differ in height has to measure all of them
to find where the next one starts, which is the difference between paging through
a library and walking it.

A selected row takes the accent fill and the accent foreground. The page
foreground on an accent fill is the usual way a themed list loses contrast, and it
looks right until the text is measured.

</div>
