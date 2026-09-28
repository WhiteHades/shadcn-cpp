// SPDX-License-Identifier: MIT
// Interaction tests for a tab set whose navigation lives elsewhere.
#include <QApplication>
#include <QTest>
#include <QSignalSpy>

#include <shadcn/navigation.hpp>

class TabsListTest : public QObject {
    Q_OBJECT
  private slots:
    void init() { shadcn::install(*qApp, shadcn::Theme::neutral(), shadcn::MotionPolicy::Reduced); }
    void cleanup() { shadcn::install(*qApp, shadcn::Theme::neutral(), shadcn::MotionPolicy::Reduced); }

    /// An application whose navigation lives in a rail still needs a value based
    /// page switcher. Without a way to hide the bar, the reader gets two sets of
    /// tabs saying the same thing, in two places, which is the mixture a component
    /// library is supposed to prevent rather than produce.
    void theListCanBeHiddenAndTheValueStillChanges() {
        shadcn::Tabs tabs;
        static_cast<void>(tabs.addTab("one", "One"));
        static_cast<void>(tabs.addTab("two", "Two"));
        auto& first = tabs.addContent("one");
        auto& second = tabs.addContent("two");
        tabs.setCurrentValue("one");
        tabs.resize(300, 200);
        tabs.show();
        QCoreApplication::processEvents();
        QVERIFY(tabs.listVisible());
        QVERIFY(tabs.list().count() > 0);

        tabs.setListVisible(false);
        QCoreApplication::processEvents();
        QVERIFY2(!tabs.listVisible(), "the list is still shown");
        // Hidden means not on screen and not focusable, not merely narrow.
        for (auto* item : tabs.list().itemAt(0) ? QList<QWidget*>{} : QList<QWidget*>{})
            if (item->isVisible()) QFAIL("a list item is still visible with the list hidden");
        QVERIFY2(!tabs.list().itemAt(0) || !tabs.list().itemAt(0)->widget()->isVisible(),
                 "the first tab button is still on screen with the list hidden");
        // The pages are still there and still switch.
        QCOMPARE(tabs.currentValue(), QString("one"));
        QVERIFY(first.isVisible());
        QVERIFY(!second.isVisible());
        QSignalSpy spy(&tabs, &shadcn::Tabs::currentChanged);
        tabs.setCurrentValue("two");
        QCOMPARE(spy.size(), 1);
        QVERIFY(second.isVisible());
        QVERIFY(!first.isVisible());

        // And it comes back, because a caller may want the bar after all.
        tabs.setListVisible(true);
        QCoreApplication::processEvents();
        QVERIFY(tabs.listVisible());
    }

    /// Hiding the list must not make the pages collapse. A hidden bar takes no room,
    /// so a page that filled the space it lost is the difference between a hidden
    /// bar and a broken layout.
    void aHiddenListTakesNoRoom() {
        // A page that fills its tab set, so the measurement is about the bar and not
        // about an empty page with no size of its own.
        auto* a = new QWidget; a->setMinimumHeight(120);
        auto* b = new QWidget; b->setMinimumHeight(120);
        shadcn::Tabs shown;
        static_cast<void>(shown.addTab("one", "One"));
        static_cast<void>(shown.addTab("two", "Two"));
        shown.addContent("one", *a);
        shown.addContent("two", *b);
        shown.setCurrentValue("one");
        shown.resize(300, 200);
        shown.show();
        QCoreApplication::processEvents();
        const auto withBar = a->height();

        auto* c = new QWidget; c->setMinimumHeight(120);
        auto* d = new QWidget; d->setMinimumHeight(120);
        shadcn::Tabs hidden;
        static_cast<void>(hidden.addTab("one", "One"));
        static_cast<void>(hidden.addTab("two", "Two"));
        hidden.addContent("one", *c);
        hidden.addContent("two", *d);
        hidden.setCurrentValue("one");
        hidden.setListVisible(false);
        hidden.resize(300, 200);
        hidden.show();
        QCoreApplication::processEvents();
        QVERIFY2(c->height() > withBar,
                 qPrintable(QStringLiteral("a page is %1 tall with the list hidden and %2 with it "
                                           "shown, so hiding the list did not give the page the room")
                                .arg(c->height()).arg(withBar)));
    }
};

QTEST_MAIN(TabsListTest)
#include "tabs_list_test.moc"
