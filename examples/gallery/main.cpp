// SPDX-License-Identifier: MIT
#include <shadcn/widgets.hpp>
#include <QApplication>
#include <QPixmap>
#include <algorithm>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>
#include <array>
#include <iostream>

int main(int argc, char** argv) {
    using namespace shadcn;
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("shadcn-cpp gallery");
    QCoreApplication::setApplicationVersion(version);
    QCommandLineParser args;
    args.setApplicationDescription("Native component gallery. Visual parity remains under review.");
    args.addHelpOption(); args.addVersionOption();
    args.addOption({"dark", "Use the dark neutral theme."});
    args.addOption({"reduced-motion", "Disable motion."});
    args.addOption({"rtl", "Use right-to-left layout."});
    args.addOption({"screenshot", "Write a PNG and exit. This is a Qt capture, not a parity baseline.", "path"});
    args.process(app);
    const bool capture = args.isSet("screenshot");
    install(app, Theme::neutral(args.isSet("dark") ? ColorMode::Dark : ColorMode::Light),
            args.isSet("reduced-motion") || capture ? MotionPolicy::Reduced : MotionPolicy::Full);
    if (args.isSet("rtl")) app.setLayoutDirection(Qt::RightToLeft);
    QWidget window;
    window.setWindowTitle("shadcn-cpp · 0.1.0");
    auto* outer = new QVBoxLayout(&window);
    outer->setContentsMargins(28, 28, 28, 28); outer->setSpacing(20);
    auto& heading = make_child<QLabel>(window, "shadcn-cpp");
    auto headingFont = heading.font(); headingFont.setPixelSize(28); headingFont.setWeight(QFont::DemiBold); heading.setFont(headingFont);
    outer->addWidget(&heading);
    auto& note = make_child<QLabel>(window, "Native C++ components. Initial port, visual review pending.");
    note.setWordWrap(true); outer->addWidget(&note);
    auto* settings = new QHBoxLayout;
    auto& dark = make_child<Switch>(window); dark.setChecked(args.isSet("dark"));
    auto& darkLabel = make_child<Label>(window, "Dark theme"); darkLabel.setBuddy(&dark);
    auto& motion = make_child<Switch>(window); motion.setChecked(args.isSet("reduced-motion") || capture);
    auto& motionLabel = make_child<Label>(window, "Reduced motion"); motionLabel.setBuddy(&motion);
    auto& rtl = make_child<Switch>(window); rtl.setChecked(args.isSet("rtl"));
    auto& rtlLabel = make_child<Label>(window, "Right to left"); rtlLabel.setBuddy(&rtl);
    settings->addWidget(&dark); settings->addWidget(&darkLabel); settings->addSpacing(20);
    settings->addWidget(&motion); settings->addWidget(&motionLabel); settings->addSpacing(20);
    settings->addWidget(&rtl); settings->addWidget(&rtlLabel); settings->addStretch(); outer->addLayout(settings);
    const auto apply = [&] {
        // Avoid replacing the application style from inside a widget's event dispatch.
        QTimer::singleShot(0, &window, [&] {
            install(app, Theme::neutral(dark.isChecked() ? ColorMode::Dark : ColorMode::Light),
                    motion.isChecked() ? MotionPolicy::Reduced : MotionPolicy::Full);
        });
    };
    QObject::connect(&dark, &QCheckBox::toggled, &window, apply);
    QObject::connect(&motion, &QCheckBox::toggled, &window, apply);
    QObject::connect(&rtl, &QCheckBox::toggled, &window, [&](bool checked) {
        app.setLayoutDirection(checked ? Qt::RightToLeft : Qt::LeftToRight);
    });
    auto& scroll = make_child<QScrollArea>(window);
    scroll.setWidgetResizable(true); scroll.setFrameShape(QFrame::NoFrame);
    auto* page = new QWidget;
    scroll.setWidget(page); // QScrollArea takes ownership.
    auto* grid = new QGridLayout(page); grid->setContentsMargins(0, 0, 12, 0); grid->setSpacing(20);
    outer->addWidget(&scroll);
    auto& buttons = make_child<Card>(*page); buttons.setTitle("Buttons");
    buttons.setDescription("Six variants with native keyboard activation.");
    constexpr std::array variants{Variant::Default, Variant::Destructive, Variant::Outline,
        Variant::Secondary, Variant::Ghost, Variant::Link};
    const std::array names{"Default", "Destructive", "Outline", "Secondary", "Ghost", "Link"};
    for (std::size_t i = 0; i < variants.size(); ++i) {
        auto& button = make_child<Button>(buttons, QString::fromLatin1(names[i]));
        button.setVariant(variants[i]); buttons.content().addWidget(&button);
    }
    auto& disabled = make_child<Button>(buttons, "Disabled"); disabled.setEnabled(false);
    buttons.content().addWidget(&disabled); grid->addWidget(&buttons, 0, 0);
    auto& course = make_child<Card>(*page); course.setTitle("Modern C++");
    course.setDescription("Lesson 7 of 18. Templates and constrained functions.");
    auto& state = make_child<Badge>(course, "In progress"); state.setVariant(Variant::Secondary);
    course.action().addWidget(&state);
    auto& name = make_child<Input>(course); name.setPlaceholderText("Search lessons");
    auto& nameLabel = make_child<Label>(course, "Lesson search"); nameLabel.setBuddy(&name);
    course.content().addWidget(&nameLabel); course.content().addWidget(&name);
    auto& progress = make_child<Progress>(course); progress.setValue(39); progress.setAccessibleName("Course progress");
    course.content().addWidget(&progress);
    auto& completion = make_child<Checkbox>(course, "Mark lesson complete"); course.content().addWidget(&completion);
    course.content().addWidget(&make_child<Separator>(course, Qt::Horizontal));
    auto& resume = make_child<Button>(course, "Continue lesson");
    auto& previous = make_child<Button>(course, "Previous"); previous.setVariant(Variant::Outline);
    course.footer().addWidget(&previous); course.footer().addStretch(); course.footer().addWidget(&resume);
    QObject::connect(&resume, &QPushButton::clicked, &course, [&] { progress.setValue(std::min(100, progress.value() + 5)); });
    QObject::connect(&previous, &QPushButton::clicked, &course, [&] { progress.setValue(std::max(0, progress.value() - 5)); });
    grid->addWidget(&course, 0, 1, Qt::AlignTop);
    auto& inputs = make_child<Card>(*page); inputs.setTitle("Input states");
    auto& error = make_child<Input>(inputs); error.setPlaceholderText("Course name"); error.setError("A course name is required.");
    auto& errorLabel = make_child<Label>(inputs, "Course name"); errorLabel.setBuddy(&error);
    inputs.content().addWidget(&errorLabel); inputs.content().addWidget(&error);
    auto& password = make_child<Input>(inputs); password.setEchoMode(QLineEdit::Password);
    password.setPlaceholderText("Password"); password.setAccessibleName("Password"); inputs.content().addWidget(&password);
    auto& loading = make_child<Skeleton>(inputs); inputs.content().addWidget(&loading);
    auto& loadingTwo = make_child<Skeleton>(inputs); loadingTwo.setMaximumWidth(190); inputs.content().addWidget(&loadingTwo);
    grid->addWidget(&inputs, 1, 0, Qt::AlignTop);
    auto& sizes = make_child<Card>(*page); sizes.setTitle("Button sizes");
    constexpr std::array sizeValues{ButtonSize::Xs, ButtonSize::Sm, ButtonSize::Default, ButtonSize::Lg,
        ButtonSize::IconXs, ButtonSize::IconSm, ButtonSize::Icon, ButtonSize::IconLg};
    const std::array sizeNames{"Extra small", "Small", "Default", "Large", "Icon xs", "Icon small", "Icon", "Icon large"};
    for (std::size_t i = 0; i < sizeValues.size(); ++i) {
        auto& row = make_child<QWidget>(sizes);
        auto* layout = new QHBoxLayout(&row); layout->setContentsMargins(0,0,0,0);
        auto& button = make_child<Button>(row, "Continue"); button.setButtonSize(sizeValues[i]);
        button.setVariant(Variant::Outline);
        // Icon-only samples intentionally have no asset. Applications supply QIcon.
        if (button_metrics(sizeValues[i]).iconOnly) button.setAccessibleName(QString::fromLatin1(sizeNames[i]));
        layout->addWidget(&button); layout->addWidget(&make_child<QLabel>(row, QString::fromLatin1(sizeNames[i]))); layout->addStretch();
        sizes.content().addWidget(&row);
    }
    grid->addWidget(&sizes, 1, 1, Qt::AlignTop);
    grid->setColumnStretch(0, 1); grid->setColumnStretch(1, 1);
    window.resize(1060, 970); window.show();
    if (capture) {
        const auto path = args.value("screenshot");
        QTimer::singleShot(250, &window, [&, path] {
            const QFileInfo destination(path);
            if (!QDir().mkpath(destination.absolutePath()) || !window.grab().save(path, "PNG")) {
                std::cerr << "Could not write screenshot\n"; app.exit(1);
            } else app.exit(0);
        });
    }
    return app.exec();
}
