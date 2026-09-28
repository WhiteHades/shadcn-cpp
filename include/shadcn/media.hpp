// SPDX-License-Identifier: MIT
#pragma once

#include <shadcn/controls.hpp>
#ifndef Q_OS_WASM
#include <QAudioOutput>
#include <QMediaPlayer>
#endif

class QLabel;
class QFileDialog;
class QTimer;
class QGraphicsOpacityEffect;
class QPropertyAnimation;

namespace shadcn {
namespace detail {
class VideoSurface;
#ifdef Q_OS_WASM
// aqtinstall publishes no Qt Multimedia module for wasm_singlethread, so the browser
// decodes. The definitions live in src/web_playback.hpp and are not part of this API.
class WebPlayback;
class WebAudioOutput;
using Playback = WebPlayback;
using AudioOutput = WebAudioOutput;
#else
using Playback = QMediaPlayer;
using AudioOutput = QAudioOutput;
#endif
} // namespace detail

/// Video playback with compact controls, captions and fullscreen. Link shadcn::media to
/// use this optional component.
///
/// A native build decodes with Qt Multimedia. The WebAssembly build has no Qt Multimedia
/// to link, so the browser decodes and the transport, timeline, settings menu and every
/// control stay in C++. See docs/components/video-player.md for what differs.
class VideoPlayer : public QWidget {
    Q_OBJECT
    Q_PROPERTY(bool fullScreen READ isFullScreen WRITE setFullScreen NOTIFY fullScreenChanged)
public:
    explicit VideoPlayer(QWidget* parent = nullptr);
    ~VideoPlayer() override;
#ifndef Q_OS_WASM
    /// Borrowed playback backend, owned by this widget.
    [[nodiscard]] QMediaPlayer& player() noexcept { return *player_; }
    /// Borrowed audio output, owned by this widget.
    [[nodiscard]] QAudioOutput& audioOutput() noexcept { return *audio_; }
#endif
    void setSource(const QUrl& source);
    [[nodiscard]] bool isFullScreen() const;
    void setFullScreen(bool enabled);
    QSize sizeHint() const override;
signals:
    void fullScreenChanged(bool enabled);
protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
private:
    void updateTransport();
    void showSettings();
    void openFile();
    void revealControls();
    QPointer<QFileDialog> fileDialog_;
    QPointer<QWidget> previousFocus_;
    QWidget* surface_;
    QWidget* controls_;
    QWidget* message_;
    QTimer* idle_;
    QGraphicsOpacityEffect* opacity_;
    QPropertyAnimation* fade_;
    detail::Playback* player_;
    detail::AudioOutput* audio_;
    detail::VideoSurface* video_;
    bool renderingFailed_ = false;
    Button* play_;
    Button* mute_;
    Button* open_;
    Button* fullscreen_;
    Slider* timeline_;
    Slider* volume_;
    QLabel* time_;
    QLabel* status_;
};

} // namespace shadcn
