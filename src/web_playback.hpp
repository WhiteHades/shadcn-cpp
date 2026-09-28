// SPDX-License-Identifier: MIT
#pragma once

// The WebAssembly playback backend for shadcn::VideoPlayer.
//
// aqtinstall publishes no Qt Multimedia module for wasm_singlethread, so the browser
// decodes. That is the arrangement Qt documents for its own WebAssembly multimedia
// support. WebPlayback drives an HTMLMediaElement through emscripten and copies each
// decoded frame into a QImage; the browser never decides when to play and never draws a
// control. Every value the widget reads comes back through one polling call, so the
// widget keeps sole ownership of playback state.

#include "media_types.hpp"

#include <QImage>
#include <QObject>
#include <QUrl>
#include <cstdint>
#include <vector>

class QTimer;

namespace shadcn::detail {

class WebPlayback final : public QObject {
    Q_OBJECT
public:
    /// The HTMLMediaElement error codes, plus one for a source the browser cannot reach.
    enum ErrorCode {
        NoError = 0,
        AbortedError = 1,
        NetworkError = 2,
        DecodeError = 3,
        UnsupportedSourceError = 4,
    };

    explicit WebPlayback(QObject* parent = nullptr);
    ~WebPlayback() override;

    void setSource(const QUrl& source);
    [[nodiscard]] QUrl source() const { return source_; }
    void play();
    void pause();
    void stop();
    [[nodiscard]] bool isPlaying() const { return playing_; }
    [[nodiscard]] bool isSeekable() const { return seekable_; }
    void setPosition(qint64 milliseconds);
    [[nodiscard]] qint64 position() const { return position_; }
    [[nodiscard]] qint64 duration() const { return duration_; }
    void setPlaybackRate(qreal rate);
    [[nodiscard]] qreal playbackRate() const { return playbackRate_; }
    [[nodiscard]] MediaStatus status() const { return status_; }
    [[nodiscard]] bool hasError() const { return errorCode_ != NoError; }
    [[nodiscard]] QString errorText() const;
    void setLooping(bool enabled);
    [[nodiscard]] bool isLooping() const { return looping_; }

    // A media element keeps its audio and caption tracks to itself, so the settings menu
    // offers nothing to choose in the browser. The menu is still built, disabled.
    [[nodiscard]] std::vector<TrackInfo> audioTracks() const { return {}; }
    [[nodiscard]] std::vector<TrackInfo> subtitleTracks() const { return {}; }
    [[nodiscard]] int activeAudioTrack() const { return -1; }
    [[nodiscard]] int activeSubtitleTrack() const { return -1; }
    void setActiveAudioTrack(int) {}
    void setActiveSubtitleTrack(int) {}

signals:
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void playbackStateChanged();
    void seekableChanged();
    void mediaStatusChanged();
    void errorOccurred();
    /// Carries the pixels the browser just decoded. Connected directly on the GUI thread.
    void frameAvailable(const QImage& frame);

private:
    friend class WebAudioOutput;
    void applyAudio(qreal volume, bool muted);
    void poll();

    QUrl source_;
    QTimer* pump_;
    std::vector<uint8_t> pixels_;
    int frameWidth_ = 0;
    int frameHeight_ = 0;
    qint64 position_ = 0;
    qint64 duration_ = 0;
    qreal playbackRate_ = 1;
    MediaStatus status_ = MediaStatus::NoMedia;
    int errorCode_ = NoError;
    bool playing_ = false;
    bool seekable_ = false;
    bool looping_ = false;
    // The element this playback owns in the browser registry, or -1 when none was made.
    int slot_ = -1;
};

/// Mirrors the browser element's volume and mute state so the volume slider reports what
/// it set. A native build gets the same behaviour from QAudioOutput.
class WebAudioOutput final : public QObject {
    Q_OBJECT
public:
    explicit WebAudioOutput(WebPlayback* playback, QObject* parent = nullptr);
    void setVolume(qreal volume);
    [[nodiscard]] qreal volume() const { return volume_; }
    void setMuted(bool muted);
    [[nodiscard]] bool isMuted() const { return muted_; }

signals:
    void volumeChanged(float volume);
    void mutedChanged(bool muted);

private:
    WebPlayback* playback_;
    qreal volume_ = .75;
    bool muted_ = false;
};

} // namespace shadcn::detail
