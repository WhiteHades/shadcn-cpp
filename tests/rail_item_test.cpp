// SPDX-License-Identifier: MIT
// Interaction tests for a rail's menu items at heights a real window reaches.
#include <QApplication>
#include <QHash>
#include <QTest>

#include <shadcn/navigation.hpp>

#include <cmath>

namespace {

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

/// The colour a region mostly holds. Sampling a point is not safe in a rail: the
/// middle of an item is a letter, the edge is its boundary, and a corner is a
/// blend. A resting item is nearly all surface, so the colour it holds most often
/// is its fill, whatever else it draws.
QColor modal(const QImage& image, const QRect& region) {
    QHash<QRgb, int> tally;
    const auto clipped = region.intersected(image.rect());
    for (int y = clipped.top(); y <= clipped.bottom(); ++y)
        for (int x = clipped.left(); x <= clipped.right(); ++x) ++tally[image.pixelColor(x, y).rgb()];
    auto best = 0;
    auto held = 0;
    for (auto it = tally.constBegin(); it != tally.constEnd(); ++it)
        if (it.value() > held) { held = it.value(); best = it.key(); }
    return QColor::fromRgb(best);
}

}  // namespace

class RailItemTest : public QObject {
    Q_OBJECT
  private slots:
    void init() { shadcn::install(*qApp, shadcn::Theme::neutral(), shadcn::MotionPolicy::Reduced); }
    void cleanup() { shadcn::install(*qApp, shadcn::Theme::neutral(), shadcn::MotionPolicy::Reduced); }

    /// A menu item is a fixed height. A button left with only a minimum will grow
    /// to fill whatever space the rail has, so two items in a tall window end up
    /// metres apart with nothing between them, and a rail of navigation stops
    /// looking like a list.
    void menuItemsKeepTheirHeightInATallRail() {
        shadcn::SidebarProvider provider;
        auto* sidebar = new shadcn::Sidebar(&provider);
        sidebar->setExpandedWidth(220);
        provider.addSidebar(*sidebar);
        // Tall enough that an expanding item has room to expand into.
        provider.resize(400, 900);
        provider.show();
        QCoreApplication::processEvents();
        auto& first = sidebar->addMenuButton("Courses", true);
        auto& second = sidebar->addMenuButton("Stats");
        QTRY_VERIFY(first.isVisible() && second.isVisible());

        const auto firstHeight = first.height();
        const auto secondHeight = second.height();
        QVERIFY2(firstHeight <= 48,
                 qPrintable(QStringLiteral("a menu item is %1 tall; it should be about 32")
                                .arg(firstHeight)));
        QVERIFY2(secondHeight <= 48,
                 qPrintable(QStringLiteral("a menu item is %1 tall; it should be about 32")
                                .arg(secondHeight)));
        // And they sit next to each other, because a list is a list.
        const auto gap = second.mapTo(sidebar, QPoint(0, 0)).y()
                       - first.mapTo(sidebar, QPoint(0, 0)).y() - firstHeight;
        QVERIFY2(gap <= 16,
                 qPrintable(QStringLiteral("%1 pixels between two items in the navigation")
                                .arg(gap)));
    }

    /// An item's text has to be readable on the rail, and on the fill it takes when
    /// it is the open one. The rail is its own surface, not the page, so a theme
    /// that paints rail text with the page's foreground is a contrast bug waiting
    /// for a rail.
    void itemTextIsReadableOnTheRailAndOnItsOwnFill() {
        for (const auto mode : {shadcn::ColorMode::Light, shadcn::ColorMode::Dark}) {
            shadcn::install(*qApp, shadcn::Theme::neutral(mode), shadcn::MotionPolicy::Reduced);
            shadcn::SidebarProvider provider;
            auto* sidebar = new shadcn::Sidebar(&provider);
            sidebar->setExpandedWidth(220);
            provider.addSidebar(*sidebar);
            provider.resize(400, 400);
            provider.show();
            QCoreApplication::processEvents();
            auto& resting = sidebar->addMenuButton("Courses");
            auto& active = sidebar->addMenuButton("Stats", true);
            QTRY_VERIFY(resting.isVisible() && active.isVisible());

            const auto rail = sidebar->grab().toImage();
            const auto* style = qobject_cast<const shadcn::Style*>(qApp->style());
            const auto theme = style->theme();
            const auto roleColour = [&theme](shadcn::Role which) {
                const auto value = theme.color(which);
                return QColor::fromRgbF(static_cast<float>(value.r), static_cast<float>(value.g),
                                        static_cast<float>(value.b));
            };
            // The open item sits on the sidebar accent, so its text is measured
            // against that fill. The strongest distance from the fill in the item is
            // its text.
            const auto fill = roleColour(shadcn::Role::SidebarAccent);
            const auto box = QRect(active.mapTo(sidebar, QPoint(0, 0)), active.size());
            auto best = 0.0;
            for (int y = box.top(); y <= box.bottom(); ++y)
                for (int x = box.left(); x <= box.right(); ++x)
                    best = std::max(best, contrast(rail.pixelColor(x, y), fill));
            QVERIFY2(best >= 4.5,
                     qPrintable(QStringLiteral("mode %1: the open item's text is %2:1 on the sidebar "
                                               "accent, needs 4.5:1")
                                    .arg(int(mode)).arg(best, 0, 'f', 2)));
            // A resting item is the rail's own surface, so a reader who looks at it
            // sees the rail and not a box.
            const auto restingBox = QRect(resting.mapTo(sidebar, QPoint(0, 0)), resting.size());
            const auto inside = modal(rail, restingBox.adjusted(4, 4, -4, -4));
            const auto beside = modal(rail, QRect(2, restingBox.y(), 4, restingBox.height()));
            QVERIFY(inside.isValid() && beside.isValid());
            QVERIFY2(contrast(inside, beside) < 1.2,
                     qPrintable(QStringLiteral("mode %1: a resting item is mostly %2 while the rail "
                                               "beside it is mostly %3")
                                    .arg(int(mode)).arg(inside.name(), beside.name())));
        }
    }
};

QTEST_MAIN(RailItemTest)
#include "rail_item_test.moc"
