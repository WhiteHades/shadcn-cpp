// SPDX-License-Identifier: MIT
#pragma once

#include <shadcn/controls.hpp>
#include <QAudioOutput>
#include <QMediaPlayer>

class QVideoWidget;
class QLabel;
class QFileDialog;

namespace shadcn {

/// Native video playback. Link shadcn::media to use this optional component.
class VideoPlayer : public QWidget {
    Q_OBJECT
    Q_PROPERTY(bool fullScreen READ isFullScreen WRITE setFullScreen NOTIFY fullScreenChanged)
public:
    explicit VideoPlayer(QWidget* parent = nullptr);
    ~VideoPlayer() override;
    /// Borrowed playback backend, owned by this widget.
    [[nodiscard]] QMediaPlayer& player() noexcept { return *player_; }
    /// Borrowed audio output, owned by this widget.
    [[nodiscard]] QAudioOutput& audioOutput() noexcept { return *audio_; }
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
    QPointer<QFileDialog> fileDialog_;
    QPointer<QWidget> previousFocus_;
    QWidget* surface_;
    QMediaPlayer* player_;
    QAudioOutput* audio_;
    QVideoWidget* video_;
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
