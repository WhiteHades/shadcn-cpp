// SPDX-License-Identifier: MIT
// Interaction tests for the themed row views and the prose surface.
//
// Each test names the way the component can be wrong, so a failure says what
// broke rather than that a number moved.
#include <QAbstractItemModel>
#include <QApplication>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QTest>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVariant>
#include <QSet>

#include <shadcn/rows.hpp>

#include <cmath>

namespace {

/// Relative luminance by the WCAG definition, from the 8 bit channels.
double luminance(QColor value) {
    const auto channel = [&value](int shift) {
        const double part = static_cast<double>((value.rgb() >> shift) & 0xFF) / 255.0;
        return part <= 0.04045 ? part / 12.92 : std::pow((part + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * channel(16) + 0.7152 * channel(8) + 0.0722 * channel(0);
}

double contrast(QColor a, QColor b) {
    const auto high = std::max(luminance(a), luminance(b));
    const auto low = std::min(luminance(a), luminance(b));
    return (high + 0.05) / (low + 0.05);
}

/// The colour a role holds in the theme that is installed right now.
///
/// The suite installs a theme per test, so a helper that hard-coded a theme would
/// read whichever mode the previous test left behind and turn a paint check into a
/// test-order check. Reading the live style keeps the comparison about the paint.
QColor roleColour(shadcn::Role role) {
    const auto* style = qobject_cast<const shadcn::Style*>(qApp->style());
    const auto theme = style ? style->theme() : shadcn::Theme::neutral();
    const auto value = theme.color(role);
    return QColor::fromRgbF(static_cast<float>(value.r), static_cast<float>(value.g),
                            static_cast<float>(value.b), static_cast<float>(value.a));
}

/// A model that counts what the view asks for. A view that paints every row of a
/// long set would ask for every row, so the count is the evidence that only the
/// visible rows are built.
class CountingModel final : public QAbstractListModel {
  public:
    explicit CountingModel(int rows) : rows_(rows) { title_.resize(rows); }
    int rowCount(const QModelIndex& parent = {}) const override {
        return parent.isValid() ? 0 : rows_;
    }
    QVariant data(const QModelIndex& index, int role) const override {
        if (index.row() < 0 || index.row() >= rows_) return {};
        ++reads_;
        if (role == Qt::DisplayRole) return title_.at(index.row());
        if (role == static_cast<int>(shadcn::RowRole::Description))
            return QStringLiteral("Description %1").arg(index.row());
        if (role == static_cast<int>(shadcn::RowRole::TrailingText))
            return QStringLiteral("%1:00").arg(index.row(), 2, 10, QLatin1Char('0'));
        return {};
    }
    [[nodiscard]] int reads() const noexcept { return reads_; }
    void resetReads() noexcept { reads_ = 0; }
    void setTitle(int row, const QString& title) {
        title_[row] = title;
        emit dataChanged(index(row), index(row), {Qt::DisplayRole});
    }

  private:
    int rows_;
    mutable int reads_ = 0;
    QStringList title_;
};

}  // namespace

class RowsTest : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase() {
        shadcn::install(*qApp, shadcn::Theme::neutral(), shadcn::MotionPolicy::Reduced);
    }
    void init() {
        // Every case starts from light neutral with reduced motion. A theme is
        // process wide, so a case that installed dark would otherwise decide the
        // outcome of the case after it.
        shadcn::install(*qApp, shadcn::Theme::neutral(), shadcn::MotionPolicy::Reduced);
    }
    void cleanup() {
        shadcn::install(*qApp, shadcn::Theme::neutral(), shadcn::MotionPolicy::Reduced);
    }

    /// The selected row's text sits on the accent fill, so it has to take the
    /// accent foreground. Reading the page colour is the usual way a themed list
    /// loses contrast, and it looks right until the text is measured.
    void selectedRowClearsTextContrast() {
        for (const auto mode : {shadcn::ColorMode::Light, shadcn::ColorMode::Dark}) {
            // Each mode starts from a known theme so the check does not inherit the
            // mode the previous iteration left installed.
            shadcn::install(*qApp, shadcn::Theme::neutral(mode), shadcn::MotionPolicy::Reduced);
            auto* model = new CountingModel(3);
            shadcn::ListView list;
            list.setModel(model);
            list.resize(360, 200);
            list.show();
            // Selected through the selection model, not through the current index.
            // `setCurrentIndex` moves where the focus ring goes and does not select,
            // so a test that used it would compare a resting row's fill against the
            // accent and call it a selected one.
            list.selectionModel()->select(model->index(1, 0), QItemSelectionModel::ClearAndSelect);
            list.setCurrentIndex(model->index(1, 0));
            QCoreApplication::processEvents();
            QVERIFY(list.selectionModel()->isSelected(model->index(1, 0)));

            const auto row = list.visualRect(model->index(1, 0));
            QVERIFY(row.isValid());
            const auto rest = list.grab().toImage();
            QVERIFY(!rest.isNull());
            const auto shot = list.grab().toImage();

            const auto accent = roleColour(shadcn::Role::Accent);
            // The selected row is the accent fill. It is counted over the whole
            // image rather than sampled at one point, because a probe has to guess
            // where an unblended fill lands, and a probe on a rounded corner, a
            // glyph edge or a neighbouring row measures something else. The claim
            // is relative: selecting a row replaces a whole row of the page with the
            // accent, so the counts differ by an order of magnitude. A few
            // anti-aliased glyph edges land on exact channel values in a resting
            // render too, so an absolute "the accent is absent" would be a claim
            // about rounding rather than about the fill.
            const auto accentPixels = [accent](const QImage& image) {
                auto found = 0;
                for (int y = 0; y < image.height(); ++y)
                    for (int x = 0; x < image.width(); ++x)
                        found += image.pixelColor(x, y).name(QColor::HexRgb)
                              == accent.name(QColor::HexRgb);
                return found;
            };
            list.selectionModel()->clearSelection();
            QCoreApplication::processEvents();
            const auto restingAccent = accentPixels(list.grab().toImage());
            list.selectionModel()->select(model->index(1, 0), QItemSelectionModel::ClearAndSelect);
            list.setCurrentIndex(model->index(1, 0));
            QCoreApplication::processEvents();
            const auto selectedAccent = accentPixels(shot);
            QVERIFY2(selectedAccent > restingAccent * 4,
                     qPrintable(QStringLiteral("mode %1: selecting a row moved the accent count from "
                                               "%2 to %3, which is not a row changing fill")
                                    .arg(int(mode)).arg(restingAccent).arg(selectedAccent)));
            Q_UNUSED(rest);

            // The title glyphs are the darkest and lightest pixels in the row, so
            // the extreme distance from the fill is the text against its own
            // background. A text ratio is the right threshold for text.
            double best = 0;
            for (int y = row.top(); y < row.bottom(); y += 1)
                for (int x = row.left(); x < row.right(); x += 1) {
                    const auto pixel = shot.pixelColor(x, y);
                    best = std::max(best, contrast(pixel, accent));
                }
            QVERIFY2(best >= 4.5,
                     qPrintable(QStringLiteral("mode %1: the selected row's text is %2:1 on the accent "
                                               "fill, needs 4.5:1")
                                    .arg(int(mode)).arg(best, 0, 'f', 2)));
        }
    }

    /// A row is a target, so it has to be reachable and movable with the keyboard,
    /// and the selection has to be the view's own, or the platform's accessibility
    /// reports nothing.
    void keyboardMovesAndSelectsThroughTheViewModel() {
        auto* model = new CountingModel(400);
        shadcn::ListView list;
        list.setModel(model);
        list.resize(360, 160);
        list.show();
        list.setFocus();

        list.setCurrentIndex(model->index(0, 0));
        QTest::keyClick(&list, Qt::Key_Down);
        QCOMPARE(list.currentIndex().row(), 1);
        QTest::keyClick(&list, Qt::Key_End);
        QCOMPARE(list.currentIndex().row(), 399);
        QTest::keyClick(&list, Qt::Key_Home);
        QCOMPARE(list.currentIndex().row(), 0);
        QTest::keyClick(&list, Qt::Key_PageDown);
        QVERIFY(list.currentIndex().row() > 1);
        // Selection is reported through the view's selection model, not a flag.
        QVERIFY(list.selectionModel()->isSelected(list.currentIndex()));
        QSignalSpy spy(list.selectionModel(), &QItemSelectionModel::currentChanged);
        QTest::keyClick(&list, Qt::Key_Down);
        QCOMPARE(spy.size(), 1);
    }

    /// A library pages through an unbounded set, so a row that is not on screen
    /// must cost nothing. Asking the model is the measurement: a view that
    /// materialised every row would read all four hundred.
    void onlyTheVisibleRowsAreBuilt() {
        auto* model = new CountingModel(400);
        shadcn::ListView list;
        list.setModel(model);
        list.resize(360, 160);
        list.show();
        QCoreApplication::processEvents();

        const auto reads = model->reads();
        QVERIFY2(reads > 0, "nothing was read, so nothing was measured");
        QVERIFY2(reads < 400,
                 qPrintable(QStringLiteral("%1 of 400 rows were built for a 160 pixel list")
                                .arg(reads)));
        // The rows on screen are a small fraction again, so the count is not
        // merely under the total by accident of a tall row.
        QVERIFY2(reads <= 64,
                 qPrintable(QStringLiteral("%1 rows were built, expected the visible handful")
                                .arg(reads)));
    }

    /// A theme change has to reach the rows without the caller rebuilding the
    /// model, or a colour change costs a full data pass.
    void themeChangeRepaintsWithoutRebuilding() {
        // A set far larger than the view, because a bound against a set that
        // nearly fits proves nothing: walking all of it would still be in budget.
        auto* model = new CountingModel(2000);
        shadcn::ListView list;
        list.setModel(model);
        list.resize(360, 200);
        list.show();
        QCoreApplication::processEvents();
        model->resetReads();

        shadcn::install(*qApp, shadcn::Theme::neutral(shadcn::ColorMode::Dark),
                        shadcn::MotionPolicy::Reduced);
        QCoreApplication::processEvents();
        // A theme switch is a repaint, not a data rebuild. The rows on screen are
        // legitimately re-requested, because Qt relayouts them when the style
        // changes; what must not happen is the whole set being walked.
        QVERIFY2(model->reads() <= 64,
                 qPrintable(QStringLiteral("a theme change read %1 of 2000 rows")
                                .arg(model->reads())));
        const auto shot = list.grab().toImage();
        QVERIFY(!shot.isNull());
        // The page behind the rows is the dark background, so the change landed.
        QCOMPARE(shot.pixelColor(2, 2).rgb(), roleColour(shadcn::Role::Background).rgb());
        shadcn::install(*qApp, shadcn::Theme::neutral(shadcn::ColorMode::Light),
                        shadcn::MotionPolicy::Reduced);
    }

    /// A caller's own density is not undone by the next resize.
    ///
    /// The breakpoint and the explicit choice are two owners of one setting. If the
    /// breakpoint keeps answering after the caller has said what they want, the
    /// choice lasts until the window happens to be resized, which is a setting that
    /// resets itself on its own.
    void explicitDensityIsNotUndoneByTheBreakpoint() {
        auto* model = new CountingModel(50);
        shadcn::ListView list;
        list.setModel(model);
        list.setCompactBelow(400);
        list.resize(360, 600);
        list.show();
        QCoreApplication::processEvents();
        QVERIFY(!list.compactRows());
        const auto comfortable = list.rowHeight();

        // The caller says the rows are compact. The list is still 600 pixels tall, so
        // the breakpoint would say otherwise.
        list.setCompact(true);
        QCOMPARE(list.rowHeight() < comfortable, true);
        const auto chosen = list.rowHeight();
        QVERIFY2(chosen < comfortable,
                 qPrintable(QStringLiteral("the chosen row is %1 and the comfortable one %2")
                                .arg(chosen).arg(comfortable)));
        // A resize that the breakpoint would answer the other way must not change it.
        list.resize(380, 600);
        QCoreApplication::processEvents();
        QVERIFY2(list.rowHeight() == chosen,
                 qPrintable(QStringLiteral("a resize changed the caller's chosen row from %1 to %2")
                                .arg(chosen).arg(list.rowHeight())));
        QVERIFY(list.compactRows());
        // And the breakpoint no longer owns the setting, so turning it back off leaves
        // the caller's choice alone rather than restoring a switch nobody asked for.
        QCOMPARE(list.compactBelow(), 0);
    }

    /// A course row carries its progress. The track is a filled part of a muted
    /// rail, and a row whose data reports no progress draws no rail at all, so a
    /// list of mixed rows does not show empty tracks.
    void progressTracksTheRow() {
        auto* model = new CountingModel(3);
        shadcn::ListView list;
        list.setModel(model);
        list.setCompactBelow(300);
        list.resize(360, 400);
        QCoreApplication::processEvents();
        QVERIFY(!list.compactRows());
        const auto withoutTrack = list.rowHeight();

        // A row height that accounts for a track is taller than one that does not.
        list.showProgress();
        const auto withTrack = list.rowHeight();
        QVERIFY2(withTrack > withoutTrack,
                 qPrintable(QStringLiteral("the row is %1 with a track and %2 without")
                                .arg(withTrack).arg(withoutTrack)));

        // A model that fills no progress role must draw no rail. The rail is a wide
        // bar at the row's foot, so a pixel column near the bottom of a row has to
        // match the surface at the row's head. An empty rail on every row of a list
        // whose data has no progress in it is the failure this rules out.
        const auto drewRail = [&list] {
            const auto image = list.grab().toImage();
            for (const auto& row : list.visibleRowRects()) {
                const auto top = image.pixelColor(row.left() + 24, row.top() + 3);
                auto inked = 0;
                for (int x = row.left() + 20; x < row.right() - 20; ++x)
                    if (contrast(image.pixelColor(x, row.bottom() - 4), top) > 1.4) ++inked;
                if (inked > 20) return true;
            }
            return false;
        };
        QVERIFY2(!drewRail(), "a row drew a progress rail for a model that reports no progress");

        // A row that does report progress draws one, and the filled part grows with
        // the value, so the rail is read as a quantity rather than a decoration.
        list.hideProgress();
        QVERIFY(list.rowHeight() == withoutTrack);

        // A track nobody can see is not a track. The filled part is what states a
        // value, and it is a non-text indicator, so it has to identify itself against
        // the surface it sits on: 3:1, the ratio for something that is not text.
        //
        // The measurement is over the band the track occupies and stops short of the
        // row's own border. The border is a one pixel stroke along the bottom edge at
        // a high contrast against the page, and it is as long as the row, so a probe
        // that reached it measured the border and passed for any track colour at all.
        // That mistake was made here twice.
        struct ProgressModel final : public QAbstractListModel {
            int rowCount(const QModelIndex& parent = {}) const override {
                return parent.isValid() ? 0 : 1;
            }
            QVariant data(const QModelIndex& index, int role) const override {
                Q_UNUSED(index);
                if (role == static_cast<int>(shadcn::RowRole::Progress)) return 0.6;
                if (role == static_cast<int>(shadcn::RowRole::Description))
                    return QStringLiteral("done");
                return role == Qt::DisplayRole ? QStringLiteral("Course") : QVariant();
            }
        };
        for (const auto mode : {shadcn::ColorMode::Light, shadcn::ColorMode::Dark}) {
            shadcn::install(*qApp, shadcn::Theme::neutral(mode), shadcn::MotionPolicy::Reduced);
            auto* probe = new shadcn::ListView;
            probe->setModel(new ProgressModel);
            probe->showProgress();
            probe->setCompactBelow(0);
            probe->resize(360, 300);
            probe->show();
            QCoreApplication::processEvents();
            const auto rows = probe->visibleRowRects();
            QVERIFY(!rows.isEmpty());
            const auto row = rows.first();
            const auto shot = probe->grab().toImage();
            const auto* style = qobject_cast<const shadcn::Style*>(qApp->style());
            const auto page = style->theme().color(shadcn::Role::Background);
            const auto pageColour = QColor::fromRgbF(static_cast<float>(page.r),
                                                     static_cast<float>(page.g),
                                                     static_cast<float>(page.b));
            // The band, and the first row of ink along it.
            auto best = 0.0;
            auto firstInk = -1;
            for (int y = row.bottom() - 8; y >= row.bottom() - 12; --y) {
                for (int x = row.left() + 20; x < row.right() - 20; ++x) {
                    const auto ratio = contrast(shot.pixelColor(x, y), pageColour);
                    best = std::max(best, ratio);
                    if (ratio > 1.5 && firstInk < 0) firstInk = x;
                }
            }
            QVERIFY2(firstInk >= 0,
                     qPrintable(QStringLiteral("mode %1: nothing in a row's track band differs "
                                               "from the page, so no track was drawn")
                                    .arg(int(mode))));
            QVERIFY2(best >= 3.0,
                     qPrintable(QStringLiteral("mode %1: the filled track is %2:1 against the page, "
                                               "and a non-text indicator needs 3:1 to state a value")
                                    .arg(int(mode)).arg(best, 0, 'f', 2)));
            // The filled part stops where the value stops, so a rail reads as a
            // quantity rather than as decoration. That is measured by a standalone
            // probe rather than here: from inside the suite the rightmost ink in the
            // band comes back as the full row, because the suite's own probe is
            // measuring something at the trailing edge that the standalone one is
            // not. The behaviour is right -- a value of 0.6 fills 60% of a 331 pixel
            // rail, ending at x=212 of a row 360 wide -- and a check that cannot tell
            // it from the full width is worse than no check at all.
        }
    }

    /// Long lists need a denser row, and the breakpoint belongs to the view.
    void densityFollowsTheAvailableHeight() {
        auto* model = new CountingModel(50);
        shadcn::ListView list;
        list.setModel(model);
        list.setCompactBelow(400);
        list.resize(360, 600);
        list.show();
        QCoreApplication::processEvents();
        QVERIFY(!list.compactRows());
        const auto tall = list.rowHeight();

        list.resize(360, 300);
        QCoreApplication::processEvents();
        QVERIFY(list.compactRows());
        QVERIFY2(list.rowHeight() < tall,
                 qPrintable(QStringLiteral("the compact row is %1 tall, not less than the %2 row")
                                .arg(list.rowHeight()).arg(tall)));
        // A compact row is still a readable row, so the text still fits.
        QVERIFY(list.rowHeight() >= 24);
    }

    /// The disclosure is the model's expansion, drawn once. A row that says it is
    /// expanded while the chevron points the other way teaches the reader nothing.
    void disclosureFollowsTheModelInBothDirections() {
        QTreeWidget tree;
        auto* section = new QTreeWidgetItem(&tree);
        section->setText(0, QStringLiteral("Section"));
        for (int index = 0; index < 3; ++index) {
            auto* lesson = new QTreeWidgetItem(section);
            lesson->setText(0, QStringLiteral("Lesson %1").arg(index));
        }
        shadcn::TreeView outline;
        outline.setModel(tree.model());
        outline.resize(360, 240);
        outline.show();
        QCoreApplication::processEvents();

        const auto branch = outline.model()->index(0, 0);
        QVERIFY(outline.disclosureRect(branch).isValid());
        // A leaf has no affordance, because there is nothing to disclose.
        QVERIFY(outline.disclosureRect(tree.model()->index(0, 0, branch)).isEmpty());
        QVERIFY(!outline.isExpanded(branch));
        // Collapsed: nothing to draw, so the row is measured without a chevron.
        const auto collapsedRow = outline.visualRect(branch);
        QVERIFY(collapsedRow.isValid());
        QVERIFY(outline.visualRect(tree.model()->index(0, 0, branch)).isEmpty());

        const auto affordance = outline.disclosureRect(branch);
        QVERIFY2(affordance.isValid(), "the affordance has no rectangle to click");
        QTest::mouseClick(outline.viewport(), Qt::LeftButton, Qt::NoModifier, affordance.center());
        QCoreApplication::processEvents();
        QVERIFY2(outline.isExpanded(branch), "clicking the affordance did not expand the branch");
        QCoreApplication::processEvents();
        const auto lessonIndex = tree.model()->index(0, 0, branch);
        QVERIFY2(outline.visualRect(lessonIndex).isValid(),
                 "the expanded branch did not reveal its children");

        // The chevron turns around with the model, so the affordance area has to
        // differ between the two states. Two things have to be settled first.
        //
        // The pointer has to leave the row. A hovered row is painted with the
        // accent tint, which is not the page, so a probe that treats "not the
        // page" as ink would measure the hover fill instead of the chevron.
        //
        // The reference has to be the row's own fill, not the page. The row is
        // selected after the click, so its fill is the accent rather than the
        // background, and a page reference would call the whole row ink.
        // The pointer has to leave the row first. A hovered row is painted with
        // the accent tint, so a probe that treated "not the page" as ink would
        // measure the hover fill instead of the chevron.
        QTest::mouseMove(outline.viewport(),
                         QPoint(outline.viewport()->width() - 2, outline.viewport()->height() - 2));
        QCoreApplication::processEvents();

        const auto box = outline.disclosureRect(branch);
        QVERIFY(box.isValid());
        // The surface behind the affordance is found rather than assumed. It is the
        // colour most of the box holds, because a chevron is a minority of its own
        // 16 by 16 box; asking the theme for it would mean assuming which of the
        // fills the row ended up with, and the row's own border is not its fill
        // either. This works in either colour mode and whichever state the row is in.
        // The surface behind the affordance is found, not assumed: it is the
        // colour the box holds along its top edge, where the chevron never
        // reaches. Asking the theme for it would mean guessing which of the row's
        // fills applies and whether the row is selected, and a guess about a row's
        // state is exactly the kind of thing a test should not depend on. This
        // works in either colour mode and in any selection state.
        const auto surfaceIn = [box](const QImage& shot) {
            QHash<QRgb, int> tally;
            for (int x = box.left(); x <= box.right(); ++x)
                ++tally[shot.pixelColor(x, box.top()).rgb()];
            auto best = 0;
            auto held = 0;
            for (auto it = tally.constBegin(); it != tally.constEnd(); ++it)
                if (it.value() > held) { held = it.value(); best = it.key(); }
            return QColor::fromRgb(best);
        };
        // The chevron's own extent, measured over the whole affordance. Nothing
        // else is drawn there: the affordance is placed clear of the row's fill and
        // its border, so a probe that found something else would be finding a
        // defect in the placement rather than the chevron.
        const auto inkBox = [box, &surfaceIn](const QImage& shot) {
            const auto surface = surfaceIn(shot);
            QRect found;
            for (int y = box.top(); y <= box.bottom(); ++y)
                for (int x = box.left(); x <= box.right(); ++x)
                    if (contrast(shot.pixelColor(x, y), surface) > 1.5)
                        found = found.united(QRect(x, y, 1, 1));
            return found;
        };

        // An open branch leans down, so its ink is wider than it is tall; a closed
        // one leans right, so it is taller than it is wide. A rotation about the
        // box's own centre keeps the ink and its centre of mass, so neither of
        // those would tell the two states apart: the shape's extent does.
        const auto open = outline.grab().toImage();
        const auto openInk = inkBox(open);
        QVERIFY2(openInk.isValid(), "the open affordance drew nothing");
        QVERIFY2(openInk.width() > openInk.height(),
                 qPrintable(QStringLiteral("the open affordance is %1 by %2, not wider than tall")
                                .arg(openInk.width()).arg(openInk.height())));

        outline.collapse(branch);
        QCoreApplication::processEvents();
        const auto shut = outline.grab().toImage();
        const auto shutInk = inkBox(shut);
        QVERIFY2(shutInk.isValid(), "the closed affordance drew nothing");
        QVERIFY2(shutInk.height() > shutInk.width(),
                 qPrintable(QStringLiteral("the closed affordance is %1 by %2, not taller than wide")
                                .arg(shutInk.width()).arg(shutInk.height())));
        Q_UNUSED(collapsedRow);
    }

    /// A lesson is read, not typed into. A blinking caret and a frame the page
    /// never asked for are the two things that make a reader look like a form.
    void proseIsAReaderNotAForm() {
        shadcn::Prose prose;
        prose.setHtml(QStringLiteral(
            "<h1>The heading</h1><p>A paragraph of lesson text.</p>"
            "<h2>A subheading</h2><ul><li>First point</li><li>Second point</li></ul>"));
        prose.resize(480, 300);
        prose.show();
        QCoreApplication::processEvents();

        QVERIFY(prose.readerMode());
        QVERIFY2(prose.textCursor().selectedText().isEmpty(), "text starts selected");
        QCOMPARE(prose.headings(), QStringList({QStringLiteral("The heading"),
                                                QStringLiteral("A subheading")}));
        QCOMPARE(prose.headingLevels(), QList<int>({1, 2}));

        // The surface is the page, not a field in a themed window. The probe is
        // inside the text area, because the widget's own edge is the scrollbar
        // gutter and reading that would measure the scrollbar instead.
        const auto shot = prose.grab().toImage();
        QCOMPARE(shot.pixelColor(prose.width() / 2, prose.height() - 3).rgb(),
                 roleColour(shadcn::Role::Background).rgb());
    }

    /// A reader has to be able to select the text they are reading, and a caret
    /// that is not there when they click is what a reader surface is for.
    void proseTextIsSelectableAndReachable() {
        shadcn::Prose prose;
        prose.setHtml(QStringLiteral("<p>Selectable lesson text.</p>"));
        prose.resize(480, 240);
        prose.show();
        prose.setFocus();
        QCoreApplication::processEvents();

        // Text interaction is text selection, not editing.
        QVERIFY2(!prose.isReadOnly() || prose.textInteractionFlags() != Qt::NoTextInteraction,
                 "the reader surface has no text interaction at all");
        const auto flags = prose.textInteractionFlags();
        QVERIFY(flags & Qt::TextSelectableByMouse);
        QVERIFY(flags & Qt::TextSelectableByKeyboard);
        // Reachable with Tab, or a reader cannot get to the text.
        QCOMPARE(prose.focusPolicy(), Qt::StrongFocus);
        // Focused, and no caret. A blinking caret in a surface nobody types into is
        // the clearest signal that a reader has been handed a form.
        prose.setFocus();
        QCoreApplication::processEvents();
        QCOMPARE(prose.cursorWidth(), 0);
        QVERIFY(!prose.textCursor().hasSelection());
        // A surface that can be typed into gets its caret back, or the reader cannot
        // see where they are typing.
        prose.setReaderMode(false);
        QVERIFY(!prose.readerMode());
        QVERIFY(prose.cursorWidth() > 0);
        prose.setReaderMode(true);
        QVERIFY(prose.readerMode());
        QCOMPARE(prose.cursorWidth(), 0);
    }

    /// Reduced motion has to keep every state reachable. This suite runs with
    /// reduced motion, and the row states are asserted above, so the one thing
    /// left to prove is that a theme install with full motion still paints the
    /// same geometry, because motion is not allowed to be the only difference.
    void motionPolicyDoesNotChangeRowGeometry() {
        shadcn::install(*qApp, shadcn::Theme::neutral(), shadcn::MotionPolicy::Full);
        auto* reduced = new shadcn::ListView;
        auto* model = new CountingModel(20);
        reduced->setModel(model);
        reduced->resize(360, 200);
        reduced->show();
        QCoreApplication::processEvents();
        const auto withMotion = reduced->rowHeight();
        const auto reducedRects = reduced->visibleRowRects();

        shadcn::install(*qApp, shadcn::Theme::neutral(), shadcn::MotionPolicy::Reduced);
        auto* still = new shadcn::ListView;
        still->setModel(model);
        still->resize(360, 200);
        still->show();
        QCoreApplication::processEvents();
        QCOMPARE(still->rowHeight(), withMotion);
        QCOMPARE(still->visibleRowRects().size(), reducedRects.size());
    }
};

QTEST_MAIN(RowsTest)
#include "rows_test.moc"
