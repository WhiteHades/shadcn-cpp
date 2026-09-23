// SPDX-License-Identifier: MIT
#include <shadcn/media.hpp>
#include "video_surface.hpp"

#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QGridLayout>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QTimer>
#include <QIconEngine>
#include <QPainter>
#include <QPainterPath>
#include <QMediaMetaData>
#include <QMenu>
#include <QShortcut>
#include <QSignalBlocker>
#include <algorithm>

namespace shadcn {
namespace {
enum class MediaGlyph { Play, Pause, Volume, Muted, Settings, Expand, Collapse };

class MediaIcon final : public QIconEngine {
public:
    explicit MediaIcon(MediaGlyph glyph) : glyph_(glyph) {}
    QIconEngine* clone() const override { return new MediaIcon(glyph_); }
    QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override {
        QPixmap image(size);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        painter.setPen(QApplication::palette().color(QPalette::ButtonText));
        paint(&painter, QRect(QPoint{}, size), mode, state);
        return image;
    }
    void paint(QPainter* painter, const QRect& rect, QIcon::Mode, QIcon::State) override {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->translate(rect.topLeft());
        painter->scale(rect.width() / 24., rect.height() / 24.);
        const auto color = painter->pen().color();
        painter->setPen(QPen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->setBrush(Qt::NoBrush);
        QPainterPath path;
        switch (glyph_) {
        case MediaGlyph::Play:
            path.moveTo(7, 4); path.lineTo(20, 12); path.lineTo(7, 20); path.closeSubpath();
            break;
        case MediaGlyph::Pause:
            path.addRoundedRect(QRectF(6, 4, 4, 16), 1, 1);
            path.addRoundedRect(QRectF(14, 4, 4, 16), 1, 1);
            break;
        case MediaGlyph::Volume:
        case MediaGlyph::Muted:
            path.moveTo(3, 9); path.lineTo(7, 9); path.lineTo(12, 5); path.lineTo(12, 19);
            path.lineTo(7, 15); path.lineTo(3, 15); path.closeSubpath();
            if (glyph_ == MediaGlyph::Muted) {
                path.moveTo(16, 9); path.lineTo(22, 15);
                path.moveTo(22, 9); path.lineTo(16, 15);
            } else {
                path.moveTo(16, 8); path.cubicTo(19, 10, 19, 14, 16, 16);
                path.moveTo(19, 5); path.cubicTo(24, 9, 24, 15, 19, 19);
            }
            break;
        case MediaGlyph::Settings:
            path.moveTo(4, 7); path.lineTo(20, 7);
            path.moveTo(4, 17); path.lineTo(20, 17);
            painter->drawPath(path); path = {};
            painter->setBrush(color);
            path.addEllipse(QPointF(9, 7), 2, 2);
            path.addEllipse(QPointF(15, 17), 2, 2);
            break;
        case MediaGlyph::Expand:
        case MediaGlyph::Collapse:
            for (int x : {0, 1}) for (int y : {0, 1}) {
                const double cornerX = x ? 20 : 4;
                const double cornerY = y ? 20 : 4;
                const double innerX = x ? 15 : 9;
                const double innerY = y ? 15 : 9;
                path.moveTo(cornerX, innerY);
                path.lineTo(glyph_ == MediaGlyph::Expand ? cornerX : innerX,
                            glyph_ == MediaGlyph::Expand ? cornerY : innerY);
                path.lineTo(innerX, cornerY);
            }
            break;
        }
        painter->drawPath(path);
        painter->restore();
    }
private:
    MediaGlyph glyph_;
};

QIcon mediaIcon(MediaGlyph glyph) { return QIcon(new MediaIcon(glyph)); }

QString timestamp(qint64 milliseconds) {
    const auto seconds = std::max(qint64{0}, milliseconds) / 1000;
    const auto minutes = seconds / 60;
    if (minutes < 60)
        return QStringLiteral("%1:%2").arg(minutes).arg(seconds % 60, 2, 10, QLatin1Char('0'));
    return QStringLiteral("%1:%2:%3").arg(minutes / 60)
        .arg(minutes % 60, 2, 10, QLatin1Char('0')).arg(seconds % 60, 2, 10, QLatin1Char('0'));
}
}

VideoPlayer::VideoPlayer(QWidget* parent) : QWidget(parent),
    surface_(new QWidget(this)), controls_(new QWidget(surface_)), message_(new QWidget(surface_)),
    idle_(new QTimer(this)), opacity_(new QGraphicsOpacityEffect(controls_)),
    fade_(new QPropertyAnimation(opacity_, "opacity", this)),
    player_(new QMediaPlayer(this)), audio_(new QAudioOutput(this)),
    video_(new detail::VideoSurface(this)), play_(new Button(tr("Play"), this)),
    mute_(new Button(tr("Mute"), this)), open_(new Button(tr("Open video"), this)),
    fullscreen_(new Button(tr("Fullscreen"), this)), timeline_(new Slider(this)),
    volume_(new Slider(0, 1, this)), time_(new QLabel(this)), status_(new QLabel(this)) {
    setAccessibleName(tr("Video player"));
    video_->setMinimumSize(160, 90);
    video_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    video_->setObjectName(QStringLiteral("videoRenderer"));
    player_->setAudioOutput(audio_);
    player_->setVideoSink(video_->videoSink());
    audio_->setVolume(.75F);
    auto* hostLayout = new QVBoxLayout(this);
    hostLayout->setContentsMargins(0, 0, 0, 0);
    hostLayout->addWidget(surface_);
    surface_->setObjectName(QStringLiteral("videoSurface"));
    surface_->setWindowTitle(tr("Video player"));
    surface_->installEventFilter(this);
    auto* layout = new QGridLayout(surface_);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(video_, 0, 0);
    auto* messageLayout = new QVBoxLayout(message_);
    message_->setMaximumWidth(480);
    messageLayout->setContentsMargins(24, 24, 24, 24);
    status_->setWordWrap(true);
    status_->setAlignment(Qt::AlignCenter);
    status_->setAccessibleName(tr("Playback status"));
    messageLayout->addWidget(status_);
    open_->setObjectName(QStringLiteral("videoOpen"));
    open_->setVariant(Variant::Outline);
    messageLayout->addWidget(open_, 0, Qt::AlignHCenter);
    layout->addWidget(message_, 0, 0, Qt::AlignCenter);
    connect(open_, &QPushButton::clicked, this, &VideoPlayer::openFile);
    timeline_->setAccessibleName(tr("Playback position"));
    timeline_->setSingleStep(1000);
    timeline_->setPageStep(10000);
    controls_->setObjectName(QStringLiteral("videoControls"));
    controls_->setAutoFillBackground(true);
    controls_->setGraphicsEffect(opacity_);
    opacity_->setOpacity(1);
    auto* controlLayout = new QVBoxLayout(controls_);
    controlLayout->setContentsMargins(12, 8, 12, 8);
    controlLayout->setSpacing(4);
    controlLayout->addWidget(timeline_);
    auto* bottom = new QWidget(surface_);
    auto* bottomLayout = new QVBoxLayout(bottom);
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(8);
    auto* captions = new QLabel(bottom);
    captions->setObjectName(QStringLiteral("videoCaptions"));
    captions->setTextFormat(Qt::PlainText);
    captions->setWordWrap(true);
    captions->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    captions->setMaximumHeight(64);
    captions->setAlignment(Qt::AlignCenter);
    captions->setAttribute(Qt::WA_TransparentForMouseEvents);
    captions->setStyleSheet(QStringLiteral("color: white; background: rgba(0,0,0,180); padding: 6px 12px;"));
    captions->hide();
    bottomLayout->addWidget(captions);
    bottomLayout->addWidget(controls_);
    layout->addWidget(bottom, 0, 0, Qt::AlignBottom);
    connect(video_->videoSink(), &QVideoSink::subtitleTextChanged, captions, [captions](const QString& text) {
        captions->setText(text);
        captions->setVisible(!text.isEmpty());
    });
    connect(video_, &QRhiWidget::renderFailed, this, [this] {
        renderingFailed_ = true;
        player_->pause();
        updateTransport();
    });
    auto* transport = new QHBoxLayout;
    transport->setContentsMargins(0, 0, 0, 0);
    transport->setSpacing(4);
    auto* settings = new Button(tr("Settings"), this);
    settings->setObjectName(QStringLiteral("videoSettings"));
    for (auto* button : {play_, mute_, settings, fullscreen_}) {
        button->setVariant(Variant::Ghost);
        button->setButtonSize(ButtonSize::IconSm);
        button->setToolTip(button->text());
    }
    play_->setIcon(mediaIcon(MediaGlyph::Play));
    mute_->setIcon(mediaIcon(MediaGlyph::Volume));
    settings->setIcon(mediaIcon(MediaGlyph::Settings));
    fullscreen_->setIcon(mediaIcon(MediaGlyph::Expand));
    play_->setObjectName(QStringLiteral("videoPlay"));
    volume_->setAccessibleName(tr("Volume"));
    volume_->setSingleStep(.05);
    volume_->setPageStep(.1);
    volume_->setMinimumWidth(32);
    volume_->setMaximumWidth(80);
    volume_->setValues({.75});
    time_->setObjectName(QStringLiteral("videoTime"));
    time_->setWordWrap(true);
    transport->addWidget(play_);
    transport->addWidget(time_);
    transport->addStretch();
    transport->addWidget(mute_);
    transport->addWidget(volume_);
    transport->addWidget(settings);
    fullscreen_->setObjectName(QStringLiteral("videoFullscreen"));
    transport->addWidget(fullscreen_);
    controlLayout->addLayout(transport);
    controls_->raise();
    message_->raise();
    idle_->setObjectName(QStringLiteral("videoControlsIdle"));
    idle_->setSingleShot(true);
    idle_->setInterval(2500);
    QEasingCurve easing(QEasingCurve::BezierSpline);
    easing.addCubicBezierSegment(QPointF(.23, 1), QPointF(.32, 1), QPointF(1, 1));
    fade_->setEasingCurve(easing);
    fade_->setDuration(160);
    connect(idle_, &QTimer::timeout, this, [this] {
        const auto* focused = QApplication::focusWidget();
        if (!player_->isPlaying() || controls_->underMouse() || fileDialog_ ||
            QApplication::activePopupWidget() || (focused && controls_->isAncestorOf(focused))) return;
        fade_->stop();
        fade_->setStartValue(opacity_->opacity());
        fade_->setEndValue(0.);
        const auto* style = qobject_cast<const Style*>(this->style());
        if (style && style->motion() == MotionPolicy::Reduced) opacity_->setOpacity(0);
        else fade_->start();
    });
    video_->setFocusPolicy(Qt::StrongFocus);
    for (auto* child : surface_->findChildren<QWidget*>()) {
        child->setMouseTracking(true);
        child->installEventFilter(this);
    }
    surface_->setMouseTracking(true);
    connect(qApp, &QApplication::focusChanged, this, [this](QWidget*, QWidget* focused) {
        if (focused && (focused == surface_ || surface_->isAncestorOf(focused))) revealControls();
    });
    connect(player_, &QMediaPlayer::playbackStateChanged, this, &VideoPlayer::revealControls);
    connect(play_, &QPushButton::clicked, this, [this] {
        if (player_->isPlaying()) player_->pause();
        else player_->play();
    });
    connect(mute_, &QPushButton::clicked, this, [this] { audio_->setMuted(!audio_->isMuted()); });
    connect(settings, &QPushButton::clicked, this, &VideoPlayer::showSettings);
    connect(fullscreen_, &QPushButton::clicked, this, [this] { setFullScreen(!isFullScreen()); });
    connect(timeline_, &Slider::valuesChanged, this, [this](const QVector<double>& values) {
        if (player_->isSeekable() && !values.isEmpty())
            player_->setPosition(static_cast<qint64>(values.first()));
    });
    connect(volume_, &Slider::valuesChanged, this, [this](const QVector<double>& values) {
        if (!values.isEmpty()) audio_->setVolume(static_cast<float>(values.first()));
    });
    connect(audio_, &QAudioOutput::volumeChanged, this, [this](float value) {
        const QSignalBlocker blocker(volume_);
        volume_->setValues({value});
    });
    connect(audio_, &QAudioOutput::mutedChanged, this, [this](bool muted) {
        mute_->setText(muted ? tr("Unmute") : tr("Mute"));
        mute_->setToolTip(mute_->text());
        mute_->setIcon(mediaIcon(muted ? MediaGlyph::Muted : MediaGlyph::Volume));
    });
    connect(player_, &QMediaPlayer::positionChanged, this, &VideoPlayer::updateTransport);
    connect(player_, &QMediaPlayer::durationChanged, this, &VideoPlayer::updateTransport);
    connect(player_, &QMediaPlayer::seekableChanged, this, &VideoPlayer::updateTransport);
    connect(player_, &QMediaPlayer::playbackStateChanged, this, &VideoPlayer::updateTransport);
    connect(player_, &QMediaPlayer::mediaStatusChanged, this, &VideoPlayer::updateTransport);
    connect(player_, &QMediaPlayer::errorOccurred, this, &VideoPlayer::updateTransport);
    const auto shortcut = [this](const QKeySequence& key, auto action) {
        auto* binding = new QShortcut(key, surface_);
        binding->setContext(Qt::WidgetWithChildrenShortcut);
        connect(binding, &QShortcut::activated, this, action);
    };
    shortcut(QKeySequence(Qt::Key_K), [this] { play_->click(); });
    shortcut(QKeySequence(Qt::Key_M), [this] { mute_->click(); });
    shortcut(QKeySequence(Qt::Key_F), [this] { setFullScreen(!isFullScreen()); });
    shortcut(QKeySequence(Qt::Key_Escape), [this] { if (isFullScreen()) setFullScreen(false); });
    shortcut(QKeySequence(Qt::Key_J), [this] {
        if (player_->isSeekable()) player_->setPosition(std::max(qint64{0}, player_->position() - 10000));
    });
    shortcut(QKeySequence(Qt::Key_L), [this] {
        if (player_->isSeekable()) player_->setPosition(std::min(player_->duration(), player_->position() + 10000));
    });
    updateTransport();
}

VideoPlayer::~VideoPlayer() {
    disconnect(qApp, nullptr, this, nullptr);
    idle_->stop();
    fade_->stop();
    for (auto* child : surface_->findChildren<QWidget*>()) child->removeEventFilter(this);
    surface_->removeEventFilter(this);
    disconnect(video_, nullptr, this, nullptr);
    disconnect(player_, nullptr, this, nullptr);
    player_->stop();
    player_->setVideoOutput(nullptr);
    player_->setAudioOutput(nullptr);
}

void VideoPlayer::setSource(const QUrl& source) {
    player_->setSource(source);
}
QSize VideoPlayer::sizeHint() const { return {640, 360}; }

bool VideoPlayer::isFullScreen() const { return surface_->isFullScreen(); }

void VideoPlayer::setFullScreen(bool enabled) {
    if (enabled == isFullScreen()) return;
    if (enabled) previousFocus_ = QApplication::focusWidget();
    auto* videoLayout = qobject_cast<QGridLayout*>(surface_->layout());
    // Reparent the renderer itself so Qt releases its old window's RHI callbacks.
    videoLayout->removeWidget(video_);
    video_->setParent(this);
    if (enabled) {
        layout()->removeWidget(surface_);
        surface_->setParent(this, Qt::Window);
    } else {
        surface_->hide();
        surface_->setWindowState(Qt::WindowNoState);
        surface_->setParent(this, Qt::Widget);
        layout()->addWidget(surface_);
    }
    videoLayout->addWidget(video_, 0, 0);
    video_->lower();
    video_->show();
    if (enabled) {
        surface_->showFullScreen();
        surface_->activateWindow();
        fullscreen_->setFocus(Qt::OtherFocusReason);
    } else {
        surface_->show();
        if (previousFocus_) previousFocus_->setFocus(Qt::OtherFocusReason);
    }
    fullscreen_->setText(enabled ? tr("Exit fullscreen") : tr("Fullscreen"));
    fullscreen_->setToolTip(fullscreen_->text());
    fullscreen_->setIcon(mediaIcon(enabled ? MediaGlyph::Collapse : MediaGlyph::Expand));
    emit fullScreenChanged(enabled);
}

bool VideoPlayer::eventFilter(QObject* watched, QEvent* event) {
    if (watched == surface_ && event->type() == QEvent::Close && isFullScreen()) {
        static_cast<QCloseEvent*>(event)->ignore();
        setFullScreen(false);
        return true;
    }
    if (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress ||
        event->type() == QEvent::KeyPress || event->type() == QEvent::Enter || event->type() == QEvent::Leave)
        revealControls();
    return QWidget::eventFilter(watched, event);
}

void VideoPlayer::revealControls() {
    fade_->stop();
    opacity_->setOpacity(1);
    if (player_->isPlaying()) idle_->start();
    else idle_->stop();
}

void VideoPlayer::updateTransport() {
    const QSignalBlocker blocker(timeline_);
    timeline_->setRange(0, static_cast<double>(std::max(qint64{1}, player_->duration())));
    timeline_->setValues({static_cast<double>(player_->position())});
    timeline_->setEnabled(player_->isSeekable());
    time_->setText(timestamp(player_->position()) + QStringLiteral(" / ") + timestamp(player_->duration()));
    const auto playLabel = player_->isPlaying() ? tr("Pause") : tr("Play");
    if (play_->text() != playLabel) {
        play_->setText(playLabel);
        play_->setToolTip(playLabel);
        play_->setIcon(mediaIcon(player_->isPlaying() ? MediaGlyph::Pause : MediaGlyph::Play));
    }
    const auto state = player_->mediaStatus();
    play_->setEnabled(!renderingFailed_ && state != QMediaPlayer::NoMedia && state != QMediaPlayer::InvalidMedia);
    QString message;
    if (renderingFailed_)
        message = tr("Cannot display video with the current graphics backend.");
    else if (player_->error() != QMediaPlayer::NoError)
        message = tr("Cannot play this video. %1").arg(player_->errorString());
    else if (state == QMediaPlayer::NoMedia) message = tr("Choose a video to start.");
    else if (state == QMediaPlayer::LoadingMedia) message = tr("Loading video…");
    else if (state == QMediaPlayer::StalledMedia || state == QMediaPlayer::BufferingMedia)
        message = tr("Buffering…");
    status_->setText(message);
    status_->setVisible(!message.isEmpty());
    open_->setVisible(!renderingFailed_ && (state == QMediaPlayer::NoMedia || state == QMediaPlayer::InvalidMedia ||
                      player_->error() != QMediaPlayer::NoError));
    open_->setText(player_->error() == QMediaPlayer::NoError ? tr("Open video") : tr("Open another file"));
    message_->setVisible(!message.isEmpty());
}

void VideoPlayer::openFile() {
    if (fileDialog_) {
        fileDialog_->raise();
        fileDialog_->activateWindow();
        return;
    }
    auto* dialog = new QFileDialog(surface_, tr("Open video"));
    fileDialog_ = dialog;
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setFileMode(QFileDialog::ExistingFile);
    dialog->setAcceptMode(QFileDialog::AcceptOpen);
    // Do not filter by suffix: supported containers depend on the installed backend.
    connect(dialog, &QFileDialog::fileSelected, this, [this](const QString& path) {
        const QPointer<VideoPlayer> alive(this);
        setSource(QUrl::fromLocalFile(path));
        if (alive) player_->play();
    });
    dialog->open();
}

void VideoPlayer::showSettings() {
    auto* menu = new QMenu(surface_);
    menu->setAttribute(Qt::WA_DeleteOnClose);
    connect(menu->addAction(tr("Open video…")), &QAction::triggered, this, &VideoPlayer::openFile);
    menu->addSeparator();
    auto* speed = menu->addMenu(tr("Speed"));
    auto* rates = new QActionGroup(speed);
    for (const auto rate : {.5, .75, 1., 1.25, 1.5, 2.}) {
        auto* action = speed->addAction(QString::number(rate) + QStringLiteral("×"));
        action->setCheckable(true);
        action->setChecked(qFuzzyCompare(player_->playbackRate(), rate));
        rates->addAction(action);
        connect(action, &QAction::triggered, this, [this, rate] { player_->setPlaybackRate(rate); });
    }
    const auto tracks = [this, menu](const QString& title, const QList<QMediaMetaData>& list,
                                   int active, bool subtitles) {
        auto* submenu = menu->addMenu(title);
        auto* group = new QActionGroup(submenu);
        const auto add = [this, submenu, group, active, subtitles](const QString& label, int index) {
            auto* action = submenu->addAction(label);
            action->setCheckable(true);
            action->setChecked(active == index);
            group->addAction(action);
            connect(action, &QAction::triggered, this, [this, index, subtitles] {
                if (subtitles) player_->setActiveSubtitleTrack(index);
                else player_->setActiveAudioTrack(index);
            });
        };
        if (subtitles) add(tr("Off"), -1);
        for (qsizetype i = 0; i < list.size(); ++i) {
            auto label = list[i].stringValue(QMediaMetaData::Title);
            if (label.isEmpty()) label = list[i].stringValue(QMediaMetaData::Language);
            if (label.isEmpty()) label = tr("Track %1").arg(i + 1);
            add(label, static_cast<int>(i));
        }
        submenu->setEnabled(!list.isEmpty());
    };
    tracks(tr("Audio"), player_->audioTracks(), player_->activeAudioTrack(), false);
    tracks(tr("Captions"), player_->subtitleTracks(), player_->activeSubtitleTrack(), true);
    auto* loop = menu->addAction(tr("Loop"));
    loop->setCheckable(true);
    loop->setChecked(player_->loops() == QMediaPlayer::Infinite);
    connect(loop, &QAction::toggled, this, [this](bool enabled) {
        player_->setLoops(enabled ? QMediaPlayer::Infinite : QMediaPlayer::Once);
    });
    auto* fill = menu->addAction(tr("Fill frame"));
    fill->setCheckable(true);
    fill->setChecked(video_->aspectRatioMode() == Qt::KeepAspectRatioByExpanding);
    connect(fill, &QAction::toggled, this, [this](bool enabled) {
        video_->setAspectRatioMode(enabled ? Qt::KeepAspectRatioByExpanding : Qt::KeepAspectRatio);
    });
    menu->popup(surface_->mapToGlobal(QPoint(surface_->width() - menu->sizeHint().width(), surface_->height())));
}

} // namespace shadcn
