// SPDX-License-Identifier: MIT
// Design source: shadcn/ui registry item.tsx, sidebar-menu.tsx and typography.
#include <shadcn/rows.hpp>

#include "focus_ring.hpp"
#include "scrollbar_style.hpp"

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextDocument>

#include <algorithm>
#include <cmath>

namespace shadcn {
namespace {

const Theme& themeFor(const QWidget& widget) {
    if (const auto* style = qobject_cast<const Style*>(widget.style())) return style->theme();
    if (const auto* style = qobject_cast<const Style*>(QApplication::style())) return style->theme();
    static const auto fallback = Theme::neutral();
    return fallback;
}
QColor colour(const QWidget& widget, Role role) {
    const auto value = themeFor(widget).color(role);
    return QColor::fromRgbF(static_cast<float>(value.r), static_cast<float>(value.g),
                            static_cast<float>(value.b), static_cast<float>(value.a));
}
QColor withAlpha(QColor value, double amount) {
    value.setAlphaF(static_cast<float>(
        std::clamp(static_cast<double>(value.alphaF()) * amount, 0.0, 1.0)));
    return value;
}
double radius(const QWidget& widget) {
    const auto fixed = widget.property("shadcnRadius");
    if (fixed.isValid()) return fixed.toDouble();
    return std::max(0.0, themeFor(widget).radius() - 2.0);
}
void rounded(QPainter& painter, const QRectF& rect, double cornerRadius, const QColor& fill,
             const QColor& border = QColor(Qt::transparent), double borderWidth = 1.0) {
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(fill);
    painter.setPen(border.alpha() == 0 ? Qt::NoPen : QPen(border, borderWidth));
    cornerRadius = std::min(cornerRadius, std::min(rect.width(), rect.height()) / 2.0);
    painter.drawRoundedRect(rect, cornerRadius, cornerRadius);
}
/// A font scaled from a base, so a text size change reaches the row rather than
/// being pinned to a point size the theme happened to set.
QFont scaledFont(const QFont& base, double factor, QFont::Weight weight) {
    auto font = base;
    if (font.pixelSize() > 0) font.setPixelSize(std::max(1, qRound(font.pixelSize() * factor)));
    else font.setPointSizeF(std::max(1.0, font.pointSizeF() * factor));
    font.setWeight(weight);
    return font;
}

/// The padding around a row's text, and the gap between the two lines. Compact
/// halves both, so a long list stays dense without becoming a different row.
int rowInset(bool compact) { return compact ? 10 : 14; }
int rowGap(bool compact) { return compact ? 8 : 16; }

/// The chevron for a branch. Drawn rather than a button, because a hit target
/// drawn on a row is already on a target, and a second widget per row is what
/// this design avoids.
void paintChevron(QPainter& painter, const QRect& box, bool open, const QColor& tint) {
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(box.center());
    painter.setPen(QPen(tint, 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    // A closed branch points right, an open one points down, so the rotation is
    // the whole difference between the two states.
    painter.rotate(open ? 90.0 : 0.0);
    // The chevron is drawn to fill most of its box. A small chevron inside a 16
    // pixel box is an anti-aliased smudge whose direction cannot be read, and a
    // reader cannot tell an open branch from a closed one.
    const auto reach = box.width() * 0.30;
    QPainterPath path;
    path.moveTo(-reach * 0.5, -reach);
    path.lineTo(reach, 0.0);
    path.lineTo(-reach * 0.5, reach);
    painter.drawPath(path);
    painter.restore();
}

}  // namespace

QVariant rowData(const QModelIndex& index, RowRole role) {
    if (!index.isValid()) return {};
    return index.data(static_cast<int>(role));
}

RowDelegate::RowDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

void RowDelegate::setCompact(bool compact) {
    if (compact_ == compact) return;
    compact_ = compact;
}

void RowDelegate::setRowFont(const QFont& font) {
    if (font_ == font) return;
    font_ = font;
}

int RowDelegate::rowHeight() const {
    // Every row is the same height, whether or not it has a description. That is
    // what lets the view build only the rows on screen: a view whose rows differ
    // has to measure all of them to know where the next one starts. A description
    // occupies the second line whether or not it has text, so a row does not
    // change height when it gains one.
    const auto line = QFontMetrics(font_).lineSpacing();
    return std::max(compact_ ? 32 : 44, line * 2 + rowGap(compact_));
}

QSize RowDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const {
    return {0, rowHeight()};
}

void RowDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                        const QModelIndex& index) const {
    // A paint without a widget has no theme to read, so it is not painted. Every
    // real paint comes from a view.
    const auto* widget = option.widget;
    if (!widget) return;
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    const auto selected = option.state.testFlag(QStyle::State_Selected);
    const auto hovered = option.state.testFlag(QStyle::State_MouseOver);
    const auto heading = rowData(index, RowRole::Heading).toBool();
    // The ring is for the keyboard, so it follows the view's own record of how it
    // was focused. A mouse user does not get a ring they did not ask for.
    const auto keyboard = widget->property("shadcnKeyboardFocus").toBool();
    const auto ringed = keyboard && option.state.testFlag(QStyle::State_HasFocus);

    // A heading labels the rows under it, so it is not a target: no fill, no
    // ring, and it never takes selection.
    if (!heading) {
        const auto rect = QRectF(option.rect).adjusted(2, 1, -2, -1);
        if (selected) {
            // The selected row is the accent fill. Its text takes the accent
            // foreground, because the page foreground on an accent fill is the
            // usual way a themed list fails contrast, and it looks correct until
            // the text is measured.
            rounded(*painter, rect, radius(*widget), colour(*widget, Role::Accent));
        } else {
            const auto fill = hovered ? withAlpha(colour(*widget, Role::Accent), .4)
                                      : colour(*widget, Role::Background);
            rounded(*painter, rect, radius(*widget), fill, colour(*widget, Role::Border));
        }
        if (ringed) {
            painter->setBrush(Qt::NoBrush);
            painter->setPen(QPen(withAlpha(colour(*widget, Role::Ring), .55), 2));
            painter->drawRoundedRect(QRectF(option.rect).adjusted(3, 2, -3, -2),
                                     radius(*widget) + 1, radius(*widget) + 1);
        }
    }

    const auto accentText = selected && !heading;
    const auto foreground =
        colour(*widget, accentText ? Role::AccentForeground : Role::Foreground);
    const auto muted = colour(*widget, accentText ? Role::AccentForeground : Role::MutedForeground);

    const auto inset = rowInset(compact_);
    const auto side = compact_ ? 16 : 20;
    auto x = option.rect.left() + inset;
    const auto right = option.rect.right() - inset;

    // The tree's own branch is drawn here rather than by a second delegate, so
    // the affordance and the row are one painting pass and cannot disagree.
    if (const auto* tree = qobject_cast<const TreeView*>(widget);
        tree && tree->disclosureRect(index).isValid()) {
        const auto box = tree->disclosureRect(index);
        paintChevron(*painter, box, tree->isExpanded(index),
                     hovered ? foreground : colour(*widget, Role::MutedForeground));
        x = box.right() + 8;
    }

    const auto leading = rowData(index, RowRole::Leading).value<QPixmap>();
    if (!leading.isNull()) {
        const auto box = QRect(x, option.rect.top() + (option.rect.height() - side) / 2, side, side);
        painter->drawPixmap(box, leading);
        x += side + 10;
    }

    // The trailing side is measured first, so the title knows how much room the
    // title does not get and cannot run under it.
    const auto bodyFont = option.font;
    const QFontMetrics bodyMetrics(bodyFont);
    const auto trailingText = rowData(index, RowRole::TrailingText).toString();
    const auto trailing = rowData(index, RowRole::Trailing).value<QPixmap>();
    auto trailingWidth = 0;
    if (!trailingText.isEmpty()) trailingWidth = bodyMetrics.horizontalAdvance(trailingText) + 12;
    if (!trailing.isNull()) trailingWidth += side;

    const auto title = index.data(Qt::DisplayRole).toString();
    const auto description = rowData(index, RowRole::Description).toString();
    const auto titleFont = scaledFont(bodyFont, heading ? 1.06 : 1.0,
                                      heading ? QFont::DemiBold : QFont::Medium);
    const QFontMetrics titleMetrics(titleFont);

    // The block is one or two lines and is centred as a unit, so a row with and
    // without a description both read as centred.
    const auto two = !description.isEmpty();
    const auto blockHeight =
        titleMetrics.lineSpacing() + (two ? bodyMetrics.lineSpacing() : 0);
    const auto available = std::max(0, right - x - trailingWidth);
    auto y = option.rect.top() + std::max(0, (option.rect.height() - blockHeight) / 2);

    painter->setFont(titleFont);
    painter->setPen(heading ? muted : foreground);
    painter->drawText(QRect(x, y, available, titleMetrics.lineSpacing()),
                      Qt::AlignLeft | Qt::AlignVCenter,
                      titleMetrics.elidedText(title, Qt::ElideRight, available));
    if (two) {
        painter->setFont(bodyFont);
        painter->setPen(muted);
        painter->drawText(QRect(x, y + titleMetrics.lineSpacing(), available,
                                bodyMetrics.lineSpacing()),
                          Qt::AlignLeft | Qt::AlignVCenter,
                          bodyMetrics.elidedText(description, Qt::ElideRight, available));
    }
    if (!trailingText.isEmpty()) {
        painter->setFont(bodyFont);
        painter->setPen(muted);
        painter->drawText(QRect(right - trailingWidth, option.rect.top(),
                                trailingWidth - 12, option.rect.height()),
                          Qt::AlignRight | Qt::AlignVCenter, trailingText);
    }
    if (!trailing.isNull()) {
        painter->drawPixmap(
            QRect(right - side, option.rect.top() + (option.rect.height() - side) / 2, side, side),
            trailing);
    }
    painter->restore();
}

void RowDensity::setCompactBelow(int height) {
    compactBelow_ = height;
    compact_ = compactBelow_ > 0;
}

bool RowDensity::apply(int viewHeight) {
    // A breakpoint of zero means the caller does not want a density switch, so
    // the rows stay at the density the caller set directly.
    const auto wanted = compactBelow_ > 0 && viewHeight < compactBelow_;
    if (wanted == compact_) return false;
    compact_ = wanted;
    return true;
}

ListView::ListView(QWidget* parent) : QListView(parent), delegate_(new RowDelegate(this)) {
    setObjectName(QStringLiteral("shadcnListView"));
    setFrameShape(QFrame::NoFrame);
    setItemDelegate(delegate_);
    setMouseTracking(true);
    // The rows are all the same height, so the view can build only the rows on
    // screen. Without this it measures every row in the set to find the next one,
    // which is a whole library walked per repaint.
    setUniformItemSizes(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setTextElideMode(Qt::ElideRight);
    setWordWrap(false);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    detail::refreshScrollBars(*this);
    viewport()->setAutoFillBackground(false);
    viewport()->setAttribute(Qt::WA_Hover, true);
    delegate_->setRowFont(font());
}

void ListView::setCompact(bool compact) {
    if (delegate_->compact() == compact) return;
    delegate_->setCompact(compact);
    scheduleDelayedItemsLayout();
    doItemsLayout();
    viewport()->update();
}

void ListView::setCompactBelow(int breakpoint) {
    if (density_.compactBelow() == breakpoint) return;
    density_.setCompactBelow(breakpoint);
    static_cast<void>(density_.apply(QListView::height()));
    delegate_->setCompact(density_.compact());
    scheduleDelayedItemsLayout();
    doItemsLayout();
    viewport()->update();
}

QList<QRect> ListView::visibleRowRects() const {
    QList<QRect> result;
    if (!model()) return result;
    // Walking stops at the first row past the viewport, so asking what is on
    // screen does not itself lay out the whole set.
    for (int row = 0; row < model()->rowCount(); ++row) {
        const auto rect = visualRect(model()->index(row, 0));
        if (!rect.isValid()) break;
        if (rect.top() >= viewport()->height()) break;
        result.append(rect);
    }
    return result;
}

void ListView::resizeEvent(QResizeEvent* event) {
    QListView::resizeEvent(event);
    if (density_.apply(QWidget::height())) {
        delegate_->setCompact(density_.compact());
        scheduleDelayedItemsLayout();
        doItemsLayout();
    }
}

bool ListView::event(QEvent* event) {
    switch (event->type()) {
    case QEvent::StyleChange:
        detail::refreshScrollBars(*this);
        // The rows are painted from theme roles, so a theme change is a repaint.
        // The rows are not re-requested, which is what keeps a theme switch off
        // the data path.
        viewport()->update();
        break;
    case QEvent::FontChange:
        delegate_->setRowFont(font());
        scheduleDelayedItemsLayout();
        doItemsLayout();
        break;
    case QEvent::FocusIn:
        setProperty("shadcnKeyboardFocus",
                    static_cast<QFocusEvent*>(event)->reason() != Qt::MouseFocusReason);
        break;
    default: break;
    }
    return QListView::event(event);
}

TreeView::TreeView(QWidget* parent) : QTreeView(parent), delegate_(new RowDelegate(this)) {
    setObjectName(QStringLiteral("shadcnTreeView"));
    setFrameShape(QFrame::NoFrame);
    setHeaderHidden(true);
    setItemDelegate(delegate_);
    setMouseTracking(true);
    setUniformRowHeights(true);
    // Qt draws its own branch indicator in the space a decorated root reserves,
    // and this tree draws the affordance itself. Left on, the row carries two
    // chevrons: Qt's at the far left and the themed one beside the text, pointing
    // at each other.
    setRootIsDecorated(false);
    // One level is the affordance, the gap after it, and the row's own inset, so
    // a child's text lands where a branch's does.
    setIndentation(disclosureSize() + 8 + rowInset(false));
    // A double click on a row should select it, not double its disclosure state
    // behind a single click the reader did not make.
    setExpandsOnDoubleClick(false);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setTextElideMode(Qt::ElideRight);
    setWordWrap(false);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    detail::refreshScrollBars(*this);
    viewport()->setAutoFillBackground(false);
    viewport()->setAttribute(Qt::WA_Hover, true);
    delegate_->setRowFont(font());
}

void TreeView::setCompact(bool compact) {
    if (delegate_->compact() == compact) return;
    delegate_->setCompact(compact);
    scheduleDelayedItemsLayout();
    doItemsLayout();
    viewport()->update();
}

void TreeView::setCompactBelow(int breakpoint) {
    if (density_.compactBelow() == breakpoint) return;
    density_.setCompactBelow(breakpoint);
    static_cast<void>(density_.apply(QTreeView::height()));
    delegate_->setCompact(density_.compact());
    scheduleDelayedItemsLayout();
    doItemsLayout();
    viewport()->update();
}

QList<QRect> TreeView::visibleRowRects() const {
    QList<QRect> result;
    if (!model()) return result;
    collectVisible(*model(), {}, result);
    return result;
}

void TreeView::collectVisible(const QAbstractItemModel& source, const QModelIndex& parent,
                              QList<QRect>& into) const {
    for (int row = 0; row < source.rowCount(parent); ++row) {
        const auto index = source.index(row, 0, parent);
        const auto rect = visualRect(index);
        if (!rect.isValid()) return;
        if (rect.top() >= viewport()->height()) return;
        into.append(rect);
        if (isExpanded(index)) collectVisible(source, index, into);
    }
}

QRect TreeView::disclosureRect(const QModelIndex& index) const {
    if (!index.isValid() || !index.model() || !index.model()->hasChildren(index)) return {};
    const auto rect = visualRect(index);
    if (!rect.isValid()) return {};
    // The affordance sits clear of the row's own chrome. The row's fill is inset
    // two pixels and stroked with a one pixel border, so an affordance placed on
    // the row's edge has a vertical stroke running beside the chevron in every
    // state, and that stroke is as tall as the affordance. It hides which way the
    // chevron points, which is the one thing the affordance exists to say.
    const auto size = disclosureSize();
    return {rect.left() + rowInset(delegate_->compact()), rect.center().y() - size / 2, size, size};
}

void TreeView::mousePressEvent(QMouseEvent* event) {
    // The affordance is a hit target inside the row, so a press on it toggles the
    // branch and does not also select the row. The press is consumed here so the
    // base class does not move the current index behind the reader's back.
    if (event->button() == Qt::LeftButton && toggleAt(event->position().toPoint())) {
        event->accept();
        return;
    }
    QTreeView::mousePressEvent(event);
}

bool TreeView::toggleAt(const QPoint& position) {
    const auto index = indexAt(position);
    if (!index.isValid()) return false;
    const auto box = disclosureRect(index);
    if (!box.contains(position)) return false;
    // Expansion belongs to the model, so a collapse survives a repaint and the
    // view never holds a second copy of the state.
    setExpanded(index, !isExpanded(index));
    return true;
}

void TreeView::resizeEvent(QResizeEvent* event) {
    QTreeView::resizeEvent(event);
    if (density_.apply(QWidget::height())) {
        delegate_->setCompact(density_.compact());
        scheduleDelayedItemsLayout();
        doItemsLayout();
    }
}

bool TreeView::event(QEvent* event) {
    switch (event->type()) {
    case QEvent::StyleChange:
        detail::refreshScrollBars(*this);
        viewport()->update();
        break;
    case QEvent::FontChange:
        delegate_->setRowFont(font());
        scheduleDelayedItemsLayout();
        doItemsLayout();
        break;
    case QEvent::FocusIn:
        setProperty("shadcnKeyboardFocus",
                    static_cast<QFocusEvent*>(event)->reason() != Qt::MouseFocusReason);
        break;
    default: break;
    }
    return QTreeView::event(event);
}

Prose::Prose(QWidget* parent) : QTextEdit(parent) {
    setObjectName(QStringLiteral("shadcnProse"));
    setFrameShape(QFrame::NoFrame);
    setReadOnly(true);
    // A reader selects text and follows links, and never types. Dropping
    // TextEditable is what removes the caret and the keyboard editing that would
    // let a reader change the lesson.
    setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard |
                            Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    setFocusPolicy(Qt::StrongFocus);
    detail::refreshScrollBars(*this);
    viewport()->setAutoFillBackground(false);
    // The surface is the page. A text edit's own base colour is the platform's
    // idea of a field, which is the one surface a reader never asked for.
    viewport()->setStyleSheet(
        QStringLiteral("background:%1").arg(colour(*this, Role::Background).name()));
    document()->setDocumentMargin(0);
    applyTypography();
}

void Prose::applyTypography() {
    const auto& theme = themeFor(*this);
    const auto hex = [this](Role role) { return colour(*this, role).name(); };
    const auto px = [this](double factor) {
        return qMax(1, qRound(font().pixelSize() > 0 ? font().pixelSize() * factor
                                                     : font().pointSizeF() * factor));
    };
    Q_UNUSED(theme);
    // A document stylesheet rather than a hand-built block layout: the text
    // engine, the wrapping and the selection stay Qt's, and only the type scale
    // and the colours are the theme's.
    const auto body = QString::number(px(1.0));
    const auto h1 = QString::number(px(1.8));
    const auto h3 = QString::number(px(1.25));
    const auto corner = QString::number(std::max(1, qRound(radius(*this))));
    auto sheet = QStringLiteral("body { color:$fg; font-size:$body; line-height:150%; }"
                                "h1 { color:$fg; font-size:$h1; font-weight:600; margin-top:0.6em; margin-bottom:0.3em; }"
                                "h2 { color:$fg; font-size:$h1; font-weight:600; margin-top:0.6em; margin-bottom:0.3em; }"
                                "h3 { color:$fg; font-size:$h3; font-weight:600; margin-top:0.5em; margin-bottom:0.25em; }"
                                "h4, h5, h6 { color:$muted; font-size:$body; font-weight:600; }"
                                "p { margin-top:0; margin-bottom:0.7em; }"
                                "a { color:$primary; text-decoration:underline; }"
                                "ul, ol { margin-top:0; margin-bottom:0.7em; margin-left:1.4em; }"
                                // Qt's rich text draws a list's marker hard against its
                                // text, so an item reads as one run. Padding on the item
                                // is the one property that moves the marker left and the
                                // text right together; margin and text-indent move both
                                // the same way and leave the gap at nothing.
                                "li { margin-bottom:0.2em; padding-left:1.1em; }"
                                "code { color:$fg; background-color:$mutedFill; padding:1px 4px; border-radius:4px; }"
                                "pre { color:$fg; background-color:$mutedFill; padding:10px 12px; border-radius:$corner; }"
                                "pre code { background-color:transparent; padding:0; }"
                                "blockquote { color:$muted; margin:0 0 0.7em 0; padding-left:12px; }"
                                "hr { color:$border; height:1px; }");
    sheet.replace("$corner", corner).replace("$h3", h3).replace("$h1", h1)
        .replace("$body", body)
        .replace("$fg", hex(Role::Foreground)).replace("$mutedFill", hex(Role::Muted))
        .replace("$muted", hex(Role::MutedForeground)).replace("$primary", hex(Role::Primary))
        .replace("$border", hex(Role::Border));
    document()->setDefaultStyleSheet(sheet);
    // The viewport is the page as well, so a link that Qt paints for itself does
    // not leave a hole in the surface.
    viewport()->setStyleSheet(
        QStringLiteral("background:%1").arg(hex(Role::Background)));
    viewport()->update();
}

QList<int> Prose::headingLevels() const {
    QList<int> levels;
    for (auto block = document()->begin(); block.isValid(); block = block.next()) {
        const auto format = block.blockFormat();
        if (format.headingLevel() > 0) levels.append(format.headingLevel());
    }
    return levels;
}

QStringList Prose::headings() const {
    QStringList found;
    for (auto block = document()->begin(); block.isValid(); block = block.next()) {
        if (block.blockFormat().headingLevel() <= 0) continue;
        auto text = block.text();
        while (text.endsWith(QLatin1Char('\n')) || text.endsWith(QLatin1Char(' ')))
            text.chop(1);
        if (!text.isEmpty()) found.append(text);
    }
    return found;
}

void Prose::setReaderMode(bool reader) {
    if (isReadOnly() == reader) return;
    // Turning reader mode off keeps the text selectable. A surface that can be
    // typed into is not a different surface, it is the same one with the caret
    // back, so selection survives either way.
    setReadOnly(reader);
    setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard |
                            (reader ? Qt::TextBrowserInteraction : Qt::TextEditable) |
                            Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
    viewport()->update();
}

bool Prose::event(QEvent* event) {
    // The type scale and the colours are derived, so a theme change rebuilds the
    // document's stylesheet. The document itself is untouched, so the text, the
    // selection and the scroll position all survive.
    if (event->type() == QEvent::StyleChange) {
        detail::refreshScrollBars(*this);
        applyTypography();
    }
    if (event->type() == QEvent::FontChange) applyTypography();
    return QTextEdit::event(event);
}

void Prose::changeEvent(QEvent* event) {
    QTextEdit::changeEvent(event);
    if (event->type() == QEvent::FontChange) applyTypography();
}

}  // namespace shadcn
