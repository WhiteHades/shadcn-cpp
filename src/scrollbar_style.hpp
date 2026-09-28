// SPDX-License-Identifier: MIT
#pragma once

// Internal: the theme's scrollbar chrome. Not a public interface; include this from
// src/ only.
//
// A scroll area and a prose surface both scroll, and a scrollbar that is 10px in
// one and 16px in the other is the kind of drift that only shows up once two
// screens sit side by side. The rule lives here so there is one rule.

#include "focus_ring.hpp"

#include <QString>

namespace shadcn::detail {

/// The stylesheet for a themed scrollbar: a transparent track so the surface
/// behind it shows, a rounded handle, and no arrow buttons, which a themed scroll
/// area has no room for and which a pointer user does not need.
inline QString scrollBarStyleSheet() {
    return QStringLiteral(
        "QScrollBar:vertical { width:10px; background:transparent; margin:0; }"
        "QScrollBar:horizontal { height:10px; background:transparent; margin:0; }"
        "QScrollBar::handle { background:%1; border-radius:5px; min-height:24px; min-width:24px; }"
        "QScrollBar::handle:hover { background:%2; }"
        "QScrollBar::handle:pressed { background:%2; }"
        "QScrollBar::add-line, QScrollBar::sub-line { width:0; height:0; }"
        "QScrollBar::add-page, QScrollBar::sub-page { background:transparent; }");
}

/// The sheet for a widget that scrolls, with the handle taken from the widget's
/// own theme. The resting handle is the border colour and the hovered handle the
/// muted colour, so a handle reads against the surface without competing with the
/// content beside it.
inline QString themedScrollBarSheet(const QWidget& widget) {
    const auto& theme = ringTheme(widget);
    return scrollBarStyleSheet().arg(ringColor(theme, Role::Border).name(QColor::HexArgb),
                                     ringColor(theme, Role::Muted).name(QColor::HexArgb));
}

/// Puts the themed scrollbar chrome on a widget, at most once per theme.
///
/// Applying a stylesheet sends `QEvent::StyleChange` to the widget it is applied
/// to, so a surface that rebuilt its sheet from inside its own style-change
/// handler would recurse until the stack ran out. The sheet only depends on the
/// theme, so it is only rewritten when the theme moved, and the answer is
/// remembered on the widget so two surfaces on screen cannot evict each other into
/// rewriting it.
inline void refreshScrollBars(QWidget& widget) {
    static thread_local QPointer<QWidget> seen;
    static thread_local const Theme* seenTheme = nullptr;
    if (seenTheme != &ringTheme(widget) || seen != &widget) {
        seen = &widget;
        seenTheme = &ringTheme(widget);
        widget.setStyleSheet(themedScrollBarSheet(widget));
    }
}

}  // namespace shadcn::detail
