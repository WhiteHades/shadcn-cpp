// SPDX-License-Identifier: MIT
#include <shadcn/widgets.hpp>
#include <QAccessible>
#include <QApplication>
#include <QCoreApplication>
#include <QImage>
#include <QPixmap>
#include <QSignalSpy>
#include <QTest>
#include <QVBoxLayout>
#include <array>
#include <memory>

class WidgetTest final : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        shadcn::install(*qobject_cast<QApplication*>(QCoreApplication::instance()),
                        shadcn::Theme::neutral(), shadcn::MotionPolicy::Reduced);
    }
    void buttonKeyboardActivation() {
        shadcn::Button button("Continue");
        button.resize(button.sizeHint()); button.show(); button.setFocus();
        QSignalSpy clicks(&button, &QPushButton::clicked);
        QTest::keyClick(&button, Qt::Key_Space);
        QCOMPARE(clicks.count(), 1);
        QTest::keyClick(&button, Qt::Key_Return);
        QCOMPARE(clicks.count(), 2);
        button.setEnabled(false);
        QTest::keyClick(&button, Qt::Key_Space);
        QTest::keyClick(&button, Qt::Key_Return);
        QCOMPARE(clicks.count(), 2);
    }
    void buttonSizesAndVariants() {
        using namespace shadcn;
        Button button("Continue");
        constexpr std::array sizes{ButtonSize::Default, ButtonSize::Xs, ButtonSize::Sm,
            ButtonSize::Lg, ButtonSize::Icon, ButtonSize::IconXs, ButtonSize::IconSm, ButtonSize::IconLg};
        for (auto size : sizes) {
            button.setButtonSize(size);
            QCOMPARE(button.buttonSize(), size);
            QVERIFY(button.sizeHint().height() >= button_metrics(size).height);
            if (button_metrics(size).iconOnly) QCOMPARE(button.sizeHint().width(), button.sizeHint().height());
        }
        for (auto variant : {Variant::Default, Variant::Destructive, Variant::Outline,
                             Variant::Secondary, Variant::Ghost, Variant::Link}) {
            button.setVariant(variant);
            QCOMPARE(button.variant(), variant);
        }
        button.setInvalid(true); QVERIFY(button.isInvalid());
    }
    void inputEditingAndErrorDescription() {
        shadcn::Input input;
        input.setText(QString::fromUtf8("Arabic العربية"));
        input.selectAll();
        QCOMPARE(input.selectedText(), input.text());
        input.insert("New text");
        QCOMPARE(input.text(), QString("New text"));
        input.undo();
        QCOMPARE(input.text(), QString::fromUtf8("Arabic العربية"));
        input.setError("A course name is required.");
        QVERIFY(input.isInvalid());
        QCOMPARE(input.accessibleDescription(), QString("A course name is required."));
        input.setError({}); QVERIFY(!input.isInvalid());
        input.setEchoMode(QLineEdit::Password);
        QVERIFY(input.displayText() != input.text());
    }
    void labelActivatesBuddy() {
        QWidget root;
        auto* layout = new QVBoxLayout(&root);
        auto& checkbox = shadcn::make_child<shadcn::Checkbox>(root, QString{});
        auto& label = shadcn::make_child<shadcn::Label>(root, "Mark complete");
        label.setBuddy(&checkbox);
        layout->addWidget(&label); layout->addWidget(&checkbox);
        root.show();
        QTest::mouseClick(&label, Qt::LeftButton);
        QVERIFY(checkbox.isChecked());
        checkbox.setEnabled(false);
        QVERIFY(!label.isEnabled());
    }
    void switchKeyboardAndPointer() {
        shadcn::Switch toggle;
        toggle.setAccessibleName("Auto play");
        toggle.resize(toggle.sizeHint()); toggle.show(); toggle.setFocus();
        QTest::keyClick(&toggle, Qt::Key_Space); QVERIFY(toggle.isChecked());
        QTest::keyClick(&toggle, Qt::Key_Return); QVERIFY(!toggle.isChecked());
        QTest::mouseClick(&toggle, Qt::LeftButton, Qt::NoModifier, QPoint(toggle.width() - 2, toggle.height() / 2));
        QVERIFY(toggle.isChecked());
        toggle.setSwitchSize(shadcn::SwitchSize::Sm);
        QCOMPARE(toggle.sizeHint(), QSize(24, 14));
    }
    void checkboxMixedState() {
        shadcn::Checkbox checkbox;
        checkbox.setTristate(true);
        checkbox.setCheckState(Qt::PartiallyChecked);
        QCOMPARE(checkbox.checkState(), Qt::PartiallyChecked);
        checkbox.click();
        QCOMPARE(checkbox.checkState(), Qt::Checked);
    }
    void nativeAccessibleControlsExist() {
        shadcn::Button button("Continue");
        auto* iface = QAccessible::queryAccessibleInterface(&button);
        QVERIFY(iface);
        QCOMPARE(iface->role(), QAccessible::Button);
        QCOMPARE(iface->text(QAccessible::Name), QString("Continue"));
        shadcn::Input input;
        QVERIFY(QAccessible::queryAccessibleInterface(&input));
    }
    void progressState() {
        shadcn::Progress progress;
        progress.setRange(0, 100); progress.setValue(25);
        QCOMPARE(progress.value(), 25);
        progress.setValue(200); // QProgressBar ignores values outside its range.
        QCOMPARE(progress.value(), 25);
        progress.setRange(0, 0);
        QCOMPARE(progress.maximum(), 0);
        progress.setRange(0, 100); progress.setValue(75);
        QCOMPARE(progress.value(), 75);
    }
    void layoutCanAdoptAParentlessComponent() {
        auto root = std::make_unique<QWidget>();
        auto* layout = new QVBoxLayout(root.get());
        auto* button = new shadcn::Button("Continue");
        QPointer<shadcn::Button> observed(button);
        layout->addWidget(button);
        QCOMPARE(button->parentWidget(), root.get());
        root->show(); button->setFocus(Qt::TabFocusReason);
        QCoreApplication::processEvents();
        QVERIFY(!root->grab().isNull());
        root.reset();
        QVERIFY(observed.isNull());
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
    void parentDestructionInvalidatesObservers() {
        auto root = std::make_unique<QWidget>();
        auto& toggle = shadcn::make_child<shadcn::Switch>(*root);
        QPointer<shadcn::Switch> observed(&toggle);
        root->show(); toggle.setChecked(true);
        root.reset();
        QVERIFY(observed.isNull());
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
    void allComponentsRenderInBothThemes() {
        // Rendering smoke test only. No browser reference or visual parity assertion.
        using namespace shadcn;
        for (auto mode : {ColorMode::Light, ColorMode::Dark}) {
            install(*qobject_cast<QApplication*>(QCoreApplication::instance()), Theme::neutral(mode), MotionPolicy::Reduced);
            QWidget root;
            auto* layout = new QVBoxLayout(&root);
            auto& card = make_child<Card>(root);
            card.setTitle("Course"); card.setDescription("Continue the current lesson.");
            layout->addWidget(&card);
            auto& content = card.content();
            content.addWidget(&make_child<Button>(card, "Continue"));
            content.addWidget(&make_child<Input>(card));
            content.addWidget(&make_child<Badge>(card, "In progress"));
            content.addWidget(&make_child<Label>(card, "Lesson name"));
            content.addWidget(&make_child<Checkbox>(card, "Complete"));
            content.addWidget(&make_child<Switch>(card));
            content.addWidget(&make_child<Separator>(card, Qt::Horizontal));
            auto& progress = make_child<Progress>(card); progress.setValue(45);
            content.addWidget(&progress);
            content.addWidget(&make_child<Skeleton>(card));
            root.resize(480, 600); root.show();
            QCoreApplication::processEvents();
            const auto image = root.grab().toImage();
            QVERIFY(!image.isNull());
            QVERIFY(image.width() >= 480);
        }
    }
};
QTEST_MAIN(WidgetTest)
#include "widgets_test.moc"
