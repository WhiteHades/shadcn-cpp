// SPDX-License-Identifier: MIT
#include <shadcn/media.hpp>
#include <QApplication>
#include <QDir>
#include <QUrl>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    shadcn::install(app, shadcn::Theme::neutral(shadcn::ColorMode::Dark));
    shadcn::VideoPlayer player;
    player.setWindowTitle(QStringLiteral("Video player"));
    player.resize(960, 620);
    player.show();
    if (app.arguments().size() > 1) {
        player.setSource(QUrl::fromUserInput(app.arguments().at(1), QDir::currentPath(),
                                           QUrl::AssumeLocalFile));
        player.player().play();
    }
    return app.exec();
}
