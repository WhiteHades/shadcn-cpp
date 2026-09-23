// SPDX-License-Identifier: MIT
#include "demos.hpp"
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <iostream>
#include <shadcn/widgets.hpp>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("shadcn-cpp gallery");
    QCoreApplication::setApplicationVersion(shadcn::version);
    QCommandLineParser args;
    args.setApplicationDescription("Native shadcn component gallery.");
    args.addHelpOption();
    args.addVersionOption();
    args.addOption({"dark", "Use the dark theme."});
    args.addOption({"reduced-motion", "Disable motion."});
    args.addOption({"rtl", "Use right to left layout."});
    args.addOption({"component", "Show one component.", "name"});
    args.addOption({"screenshot", "Save the visible component as a PNG.", "path"});
    args.addOption({"capture-all", "Save every component as a PNG.", "directory"});
    args.process(app);
    const bool capture = args.isSet("screenshot") || args.isSet("capture-all");
    const bool dark = args.isSet("dark");
    shadcn::install(
        app, shadcn::Theme::neutral(dark ? shadcn::ColorMode::Dark : shadcn::ColorMode::Light),
        capture || args.isSet("reduced-motion") ? shadcn::MotionPolicy::Reduced
                                                : shadcn::MotionPolicy::Full);
    if (args.isSet("rtl"))
        app.setLayoutDirection(Qt::RightToLeft);
    const auto names = gallery::components();
    if (args.isSet("component") && !names.contains(args.value("component"))) {
        std::cerr << "Unknown component\n";
        return 1;
    }
    if (args.isSet("capture-all")) {
        const QDir output(args.value("capture-all"));
        if (!QDir().mkpath(output.absolutePath()))
            return 1;
        for (const auto& name : names) {
            auto* preview = gallery::demo(name);
            preview->resize(640, name == "card" ? 360 : 300);
            preview->show();
            QApplication::processEvents();
            preview->setFocusPolicy(Qt::ClickFocus);
            preview->setFocus(Qt::MouseFocusReason);
            QApplication::processEvents();
            const bool saved = gallery::capture(*preview).save(
                output.filePath(name + (dark ? "-dark.png" : "-light.png")), "PNG");
            delete preview;
            if (!saved) {
                std::cerr << "Could not save preview\n";
                return 1;
            }
        }
        return 0;
    }
    QWidget* window = nullptr;
    if (args.isSet("component")) {
        window = gallery::demo(args.value("component"));
        window->resize(640, 360);
    } else {
        window = new QWidget;
        auto* outer = new QHBoxLayout(window);
        outer->setContentsMargins(0, 0, 0, 0);
        outer->setSpacing(0);
        auto* navigation = new QListWidget(window);
        navigation->setFixedWidth(200);
        navigation->setFrameShape(QFrame::NoFrame);
        navigation->setStyleSheet("QListWidget { padding: 20px 8px; } QListWidget::item { padding: "
                                  "8px 12px; border-radius: 6px; }");
        auto* pages = new QStackedWidget(window);
        for (const auto& name : names) {
            auto title = name;
            title.replace('-', ' ');
            title[0] = title[0].toUpper();
            navigation->addItem(title);
            pages->addWidget(gallery::demo(name, pages));
        }
        outer->addWidget(navigation);
        outer->addWidget(new shadcn::Separator(Qt::Vertical, window));
        outer->addWidget(pages, 1);
        QObject::connect(navigation, &QListWidget::currentRowChanged, pages,
                         &QStackedWidget::setCurrentIndex);
        navigation->setCurrentRow(static_cast<int>(names.indexOf("button")));
        window->resize(960, 600);
    }
    window->setWindowTitle("shadcn-cpp");
#ifdef Q_OS_WASM
    window->showFullScreen();
    window->setFocusPolicy(Qt::ClickFocus);
    window->setFocus(Qt::MouseFocusReason);
#else
    window->show();
#endif
    if (args.isSet("screenshot")) {
        const auto path = args.value("screenshot");
        QTimer::singleShot(150, window, [&, path] {
            const QFileInfo target(path);
            app.exit(QDir().mkpath(target.absolutePath()) && gallery::capture(*window).save(path, "PNG") ? 0
                                                                                              : 1);
        });
    }
    const int result = app.exec();
    delete window;
    return result;
}
