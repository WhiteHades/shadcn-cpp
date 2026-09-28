// SPDX-License-Identifier: MIT
// Design source: shadcn/ui registry item.tsx, sidebar-menu.tsx and typography.
#pragma once

#include <shadcn/widgets.hpp>

#include <QListView>
#include <QStyledItemDelegate>
#include <QTextEdit>
#include <QTreeView>
#include <QVariant>

class QResizeEvent;

namespace shadcn {

/// The data a themed view reads for one row.
///
/// A caller supplies data, not widgets, so a large set costs only the rows on
/// screen. The title comes from `Qt::DisplayRole`, so a model that fills nothing
/// else still produces a readable row.
enum class RowRole : int {
    Description = Qt::UserRole + 1,  ///< The muted line under the title.
    Leading,                          ///< A `QPixmap` drawn before the text.
    Trailing,                         ///< A `QPixmap` drawn after the text.
    TrailingText,                     ///< A short muted label on the trailing side.
    Heading,                          ///< A row that labels the rows under it, not a target.
};

[[nodiscard]] QVariant rowData(const QModelIndex& index, RowRole role);

/// Paints one item row from theme roles. Both views share it, so a row is
/// described once rather than once per view.
class RowDelegate final : public QStyledItemDelegate {
    Q_OBJECT
  public:
    explicit RowDelegate(QObject* parent = nullptr);
    /// A compact row is shorter and uses a smaller inset, so a long list stays
    /// dense. It is the same row at a different density, not a different row.
    void setCompact(bool compact);
    [[nodiscard]] bool compact() const noexcept { return compact_; }
    /// The height one row occupies at the current density, for a caller doing its
    /// own scrolling. It reads the delegate's own font, so a view hands it the
    /// view's font and the two agree.
    [[nodiscard]] int rowHeight() const;
    /// The font the row height is measured from. `QStyledItemDelegate` keeps one
    /// for its own metrics; the row height has to be measured from the same one
    /// or the view lays rows out at a height the delegate disagrees with.
    void setRowFont(const QFont& font);
    [[nodiscard]] const QFont& rowFont() const noexcept { return font_; }
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

  private:
    bool compact_ = false;
    QFont font_;
};

/// When a list drops to the compact row.
///
/// The breakpoint lives here rather than in each view, because a breakpoint the
/// caller and the view both hold is a breakpoint that disagrees with itself. The
/// view answers the question with its own height on every resize.
class RowDensity final {
  public:
    /// Below this height the rows become compact. Zero disables the switch.
    void setCompactBelow(int height);
    [[nodiscard]] int compactBelow() const noexcept { return compactBelow_; }
    /// Sets the density directly, for a caller that has already decided.
    void setCompact(bool compact) { compact_ = compact; }
    [[nodiscard]] bool compact() const noexcept { return compact_; }
    /// Answers the breakpoint for a view of the given height. Returns true when
    /// the density changed, so a caller can re-lay out only then.
    [[nodiscard]] bool apply(int viewHeight);

  private:
    int compactBelow_ = 0;
    bool compact_ = false;
};

/// A virtualised list of item rows. Selection, hover and the focus ring come from
/// the view, so Qt's selection model and the platform's accessibility work
/// without a widget per row.
class ListView : public QListView {
    Q_OBJECT
  public:
    explicit ListView(QWidget* parent = nullptr);
    void setCompact(bool compact);
    [[nodiscard]] bool compactRows() const noexcept { return delegate_->compact(); }
    void setCompactBelow(int height);
    [[nodiscard]] int compactBelow() const noexcept { return density_.compactBelow(); }
    /// The height one row occupies at the current density.
    [[nodiscard]] int rowHeight() const { return delegate_->rowHeight(); }
    /// The rectangles of the rows on screen, in order, for a test or a hit check.
    [[nodiscard]] QList<QRect> visibleRowRects() const;

  protected:
    void resizeEvent(QResizeEvent* event) override;
    bool event(QEvent* event) override;

  private:
    RowDelegate* delegate_ = nullptr;
    RowDensity density_;
};

/// The same rows for nested content, with a disclosure affordance taken from the
/// model's own expansion state rather than a second copy of it.
class TreeView : public QTreeView {
    Q_OBJECT
  public:
    explicit TreeView(QWidget* parent = nullptr);
    void setCompact(bool compact);
    [[nodiscard]] bool compactRows() const noexcept { return delegate_->compact(); }
    void setCompactBelow(int height);
    [[nodiscard]] int compactBelow() const noexcept { return density_.compactBelow(); }
    /// The height one row occupies at the current density.
    [[nodiscard]] int rowHeight() const { return delegate_->rowHeight(); }
    /// The rectangles of the rows on screen, in order, for a test or a hit check.
    [[nodiscard]] QList<QRect> visibleRowRects() const;
    /// The affordance for a row, empty when the row has no children. Clicking it
    /// toggles the model.
    [[nodiscard]] QRect disclosureRect(const QModelIndex& index) const;
    [[nodiscard]] static constexpr int disclosureSize() noexcept { return 16; }

  protected:
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool event(QEvent* event) override;

  private:
    /// Toggles the branch under a press. Returns true when the press landed on an
    /// affordance, so the caller knows the row was not also selected.
    bool toggleAt(const QPoint& position);
    /// Walks the expanded rows on screen. It stops at the first row past the
    /// viewport, so asking what is visible does not lay out the whole tree.
    void collectVisible(const QAbstractItemModel& source, const QModelIndex& parent,
                        QList<QRect>& into) const;
    RowDelegate* delegate_ = nullptr;
    RowDensity density_;
};

/// A read-only, theme-coloured surface for a document body.
///
/// A stock `QTextEdit` is not this: it brings the platform's frame, its own text
/// colour, a blinking caret and a selection the page never asked for. The text
/// layout is Qt's, because a document engine is not this library's work; the
/// surface around it is the theme's.
class Prose : public QTextEdit {
    Q_OBJECT
  public:
    explicit Prose(QWidget* parent = nullptr);
    /// Rebuilds the document stylesheet from the theme. Called on a theme change
    /// and on a font change, so a caller never maintains it.
    void applyTypography();
    /// The heading levels the document exposes, in order, for a caller that needs
    /// the structure rather than the text.
    [[nodiscard]] QList<int> headingLevels() const;
    /// The text of each heading block, in order, for a table of contents.
    [[nodiscard]] QStringList headings() const;
    /// True when the surface cannot be typed into. Text stays selectable in this
    /// mode, because a reader has to be able to copy from the lesson.
    [[nodiscard]] bool readerMode() const noexcept { return isReadOnly(); }
    void setReaderMode(bool reader);

  protected:
    bool event(QEvent* event) override;
    void changeEvent(QEvent* event) override;
};

}  // namespace shadcn
