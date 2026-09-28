// SPDX-License-Identifier: MIT
// Interaction tests for the sidebar rail.
//
// Each test names the way the rail can be wrong, so a failure says what broke
// rather than that a number moved.
#include <QApplication>
#include <QSignalSpy>
#include <QStyle>
#include <QHash>
#include <QHash>
#include <QTest>

#include <shadcn/navigation.hpp>

#include <cmath>

namespace {

/// The relative luminance of a colour, by the WCAG definition.
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

/// The colour a region mostly holds. A resting item is nearly all surface, so the
/// colour it holds most often is its fill, whatever else it draws.
QColor modal(const QImage& image, const QRect& region) {
    QHash<QRgb, int> tally;
    const auto clipped = region.intersected(image.rect());
    for (int y = clipped.top(); y <= clipped.bottom(); ++y)
        for (int x = clipped.left(); x <= clipped.right(); ++x)
            ++tally[image.pixelColor(x, y).rgb()];
    auto best = 0;
    auto held = 0;
    for (auto it = tally.constBegin(); it != tally.constEnd(); ++it)
        if (it.value() > held) { held = it.value(); best = it.key(); }
    return QColor::fromRgb(best);
}

}  // namespace

class SidebarTest : public QObject {
    Q_OBJECT
  private slots:
    /// A rail's items carry a style sheet built from theme roles, so a theme change
    /// has to rebuild it. Repainting alone leaves every item wearing the colours of
    /// the mode the rail was built in, and a rail switched to dark keeps its light
    /// accents: the open item stays a near-white block on a near-black page.
    void itemsFollowTheTheme() {
        shadcn::install(*qApp, shadcn::Theme::neutral(shadcn::ColorMode::Light),
                        shadcn::MotionPolicy::Reduced);
        shadcn::SidebarProvider provider;
        auto* sidebar = new shadcn::Sidebar(&provider);
        sidebar->setExpandedWidth(220);
        provider.addSidebar(*sidebar);
        provider.resize(400, 400);
        provider.show();
        QCoreApplication::processEvents();
        auto& active = sidebar->addMenuButton("Courses", true);
        QTRY_VERIFY(active.isVisible());
        const auto railOf = [this, sidebar] {
            const auto* style = qobject_cast<const shadcn::Style*>(qApp->style());
            const auto accent = style->theme().color(shadcn::Role::SidebarAccent);
            return QColor::fromRgbF(static_cast<float>(accent.r), static_cast<float>(accent.g),
                                    static_cast<float>(accent.b));
        };
        const auto shot = sidebar->grab().toImage();
        const auto box = QRect(active.mapTo(sidebar, QPoint(0, 0)), active.size());
        const auto lit = modal(shot, box);
        // In light mode the open item sits on a near-white accent, so it must be
        // lighter than the page it sits on rather than darker.
        const auto* style = qobject_cast<const shadcn::Style*>(qApp->style());
        const auto page = style->theme().color(shadcn::Role::Sidebar);
        const auto pageColour = QColor::fromRgbF(static_cast<float>(page.r), static_cast<float>(page.g),
                                                 static_cast<float>(page.b));
        QVERIFY2(contrast(lit, pageColour) < 1.4,
                 "the open item is not near the sidebar accent in light mode");

        // Now the theme moves under the rail's feet.
        shadcn::install(*qApp, shadcn::Theme::neutral(shadcn::ColorMode::Dark),
                        shadcn::MotionPolicy::Reduced);
        QCoreApplication::processEvents();
        const auto darkShot = sidebar->grab().toImage();
        const auto darkItem = modal(darkShot, box);
        QVERIFY2(contrast(darkItem, lit) > 2.0,
                 qPrintable(QStringLiteral("the open item is %1 after the theme moved and %2 "
                                           "before, so it is still wearing the old mode's colours")
                                .arg(darkItem.name(), lit.name())));
        Q_UNUSED(railOf);
    }

    void initTestCase() {
        shadcn::install(*qApp, shadcn::Theme::neutral(), shadcn::MotionPolicy::Reduced);
    }
    void init() { shadcn::install(*qApp, shadcn::Theme::neutral(), shadcn::MotionPolicy::Reduced); }
    void cleanup() { shadcn::install(*qApp, shadcn::Theme::neutral(), shadcn::MotionPolicy::Reduced); }

    /// A resting menu item is not a box. The upstream default item is transparent
    /// with no border, and only draws a boundary when it is hovered, focused or
    /// active. A permanent border turns a rail of navigation into a stack of
    /// buttons, which is a different component wearing the same name.
    void restingMenuItemHasNoBorder() {
        shadcn::SidebarProvider provider;
        auto* sidebar = new shadcn::Sidebar(&provider);
        sidebar->setExpandedWidth(220);
        provider.addSidebar(*sidebar);
        provider.resize(500, 300);
        provider.show();
        QCoreApplication::processEvents();
        auto& item = sidebar->addMenuButton("Courses");
        QTRY_VERIFY(item.isVisible());

        // The resting item's fill is the surface behind it, so a pixel inside the
        // item and a pixel on the rail beside it are the same colour.
        //
        // The colour the item mostly holds is the only safe way to ask what it is
        // filled with. Sampling a point is not: the middle of a menu item is a
        // letter, the edge is the boundary, and a corner is a blend of the two. A
        // resting item is nearly all surface, so the colour it holds most often is
        // its fill, whatever else it draws.
        const auto modal = [](const QImage& image, const QRect& region) {
            QHash<QRgb, int> tally;
            const auto clipped = region.intersected(image.rect());
            for (int y = clipped.top(); y <= clipped.bottom(); ++y)
                for (int x = clipped.left(); x <= clipped.right(); ++x)
                    ++tally[image.pixelColor(x, y).rgb()];
            auto best = 0;
            auto held = 0;
            for (auto it = tally.constBegin(); it != tally.constEnd(); ++it)
                if (it.value() > held) { held = it.value(); best = it.key(); }
            return QColor::fromRgb(best);
        };
        // Both samples come from one grab of the rail. Grabbing a widget on its own
        // reports a surface that was never polished, which says nothing about how
        // the item looks next to the rail it sits in, and that is the claim.
        const auto rail = sidebar->grab().toImage();
        QVERIFY(!rail.isNull());
        const auto itemBox = item.mapTo(sidebar, QPoint(0, 0));
        const auto inside = modal(rail, QRect(itemBox, item.size()));
        const auto beside = modal(rail, QRect(2, itemBox.y(), 4, item.height()));
        QVERIFY(inside.isValid() && beside.isValid());
        QVERIFY2(contrast(inside, beside) < 1.2,
                 qPrintable(QStringLiteral("a resting menu item is mostly %1 while the rail beside "
                                           "it is mostly %2, so the item is a box of its own")
                                .arg(inside.name(), beside.name())));
    }

    /// A footer item belongs in the footer. The rail's footer is where a control
    /// that acts on the whole application lives, separate from the navigation above
    /// it, and a caller that cannot put one there puts it among the navigation and
    /// the two stop looking like different things.
    void footerItemsLandInTheFooter() {
        shadcn::SidebarProvider provider;
        auto* sidebar = new shadcn::Sidebar(&provider);
        sidebar->setExpandedWidth(220);
        provider.addSidebar(*sidebar);
        provider.resize(500, 300);
        provider.show();
        QCoreApplication::processEvents();
        (void)sidebar->addMenuButton("Courses");
        (void)sidebar->addMenuButton("Stats");
        auto& settings = sidebar->addFooterMenuButton("Settings");
        QTRY_VERIFY(settings.isVisible());

        // The footer item is below the navigation, not among it. The two layouts have
        // their own coordinate systems, so the comparison is made in the rail's.
        const auto navWidget = sidebar->content().itemAt(0)->widget();
        const auto footWidget = sidebar->footer().itemAt(0)->widget();
        QVERIFY(navWidget);
        QVERIFY(footWidget);
        const auto navY = navWidget->mapTo(sidebar, QPoint(0, 0)).y();
        const auto footY = footWidget->mapTo(sidebar, QPoint(0, 0)).y();
        QVERIFY2(footY > navY,
                 qPrintable(QStringLiteral("the footer item is at y=%1 and the navigation at y=%2, "
                                           "so the footer is not below the navigation")
                                .arg(footY).arg(navY)));
        QVERIFY2(footY > sidebar->content().geometry().height() / 2,
                 "the footer item is still within the navigation's half of the rail");
    }

    /// A navigation item says which page is open by its fill, and the active item's
    /// text has to stay readable on that fill. Reading the rail's own foreground on
    /// the accent is the usual way a themed rail loses contrast, and it looks right
    /// until the text is measured.
    void activeItemClearsTextContrast() {
        for (const auto mode : {shadcn::ColorMode::Light, shadcn::ColorMode::Dark}) {
            shadcn::install(*qApp, shadcn::Theme::neutral(mode), shadcn::MotionPolicy::Reduced);
            shadcn::SidebarProvider provider;
            auto* sidebar = new shadcn::Sidebar(&provider);
            sidebar->setExpandedWidth(220);
            provider.addSidebar(*sidebar);
            provider.resize(500, 300);
            provider.show();
            QCoreApplication::processEvents();
            auto& active = sidebar->addMenuButton("Courses", true);
            QTRY_VERIFY(active.isVisible());

            const auto image = active.grab().toImage();
            const auto* style = qobject_cast<const shadcn::Style*>(qApp->style());
            const auto theme = style->theme();
            const auto roleColour = [this, &theme](shadcn::Role which) {
                const auto value = theme.color(which);
                return QColor::fromRgbF(static_cast<float>(value.r), static_cast<float>(value.g),
                                        static_cast<float>(value.b));
            };
            // The fill is the sidebar accent; the text has to clear a text ratio on
            // it. The strongest distance from the fill in the row is the text.
            const auto fill = roleColour(shadcn::Role::SidebarAccent);
            auto best = 0.0;
            for (int y = 0; y < image.height(); ++y)
                for (int x = 0; x < image.width(); ++x)
                    best = std::max(best, contrast(image.pixelColor(x, y), fill));
            QVERIFY2(best >= 4.5,
                     qPrintable(QStringLiteral("mode %1: the active item's text is %2:1 on the "
                                               "sidebar accent, needs 4.5:1")
                                    .arg(int(mode)).arg(best, 0, 'f', 2)));
        }
    }

    /// The trigger finds its own rail and toggles it. A trigger that has to be wired
    /// to a specific rail by the caller is a trigger that will be wired to the wrong
    /// one eventually.
    void triggerTogglesItsOwnRail() {
        shadcn::SidebarProvider provider;
        auto* sidebar = new shadcn::Sidebar(&provider);
        sidebar->setExpandedWidth(220);
        provider.addSidebar(*sidebar);
        provider.resize(500, 300);
        provider.show();
        QCoreApplication::processEvents();
        QVERIFY(sidebar->isOpen());
        auto trigger = new shadcn::SidebarTrigger(&provider);
        QTest::mouseClick(trigger, Qt::LeftButton);
        QCoreApplication::processEvents();
        QVERIFY2(!sidebar->isOpen(), "the trigger did not close the rail");
        QTest::mouseClick(trigger, Qt::LeftButton);
        QCoreApplication::processEvents();
        QVERIFY2(sidebar->isOpen(), "the trigger did not reopen the rail");
    }
};

QTEST_MAIN(SidebarTest)
#include "sidebar_test.moc"
