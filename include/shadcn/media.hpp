// SPDX-License-Identifier: MIT
#pragma once

#include <shadcn/controls.hpp>
#include <QAudioOutput>
#include <QMediaPlayer>

class QVideoWidget;
class QLabel;

namespace shadcn {

/// Native video playback. Link shadcn::media to use this optional component.
class VideoPlayer : public QWidget {
    Q_OBJECT
public:
    explicit VideoPlayer(QWidget* parent = nullptr);
    ~VideoPlayer() override;
    /// Borrowed playback backend, owned by this widget.
    [[nodiscard]] QMediaPlayer& player() noexcept { return *player_; }
    /// Borrowed audio output, owned by this widget.
    [[nodiscard]] QAudioOutput& audioOutput() noexcept { return *audio_; }
    void setSource(const QUrl& source);
    QSize sizeHint() const override;
private:
    void updateTransport();
    void showSettings();
    QMediaPlayer* player_;
    QAudioOutput* audio_;
    QVideoWidget* video_;
    Button* play_;
    Button* mute_;
    Slider* timeline_;
    Slider* volume_;
    QLabel* time_;
    QLabel* status_;
};

} // namespace shadcn
