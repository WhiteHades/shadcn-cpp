// SPDX-License-Identifier: MIT
#pragma once

// The playback seam for shadcn::VideoPlayer.
//
// A native build drives Qt Multimedia directly, so Playback is QMediaPlayer and every
// helper below is a thin reading of that class. The WebAssembly build has no Qt Multimedia
// to drive: aqtinstall publishes no multimedia module for wasm_singlethread, so the browser
// decodes and WebPlayback forwards that decode over an emscripten bridge. Keeping the seam
// this small means the widget, its transport and its tests are written once.
//
// The helpers are free functions so the widget reads the same on both platforms. There is
// no virtual dispatch and no registration: one definition per platform, selected by the
// preprocessor.

#include "media_types.hpp"

#ifdef Q_OS_WASM
class QTimer;
#else
#include <QAudioOutput>
#include <QList>
#include <QMediaMetaData>
#include <QMediaPlayer>
#endif

#ifdef Q_OS_WASM
#include "web_playback.hpp"
#endif

namespace shadcn::detail {

#ifdef Q_OS_WASM

// The names the widget and the backend share. The public header declares the same
// aliases, so a consumer of shadcn::VideoPlayer reads one type either way.
using Playback = WebPlayback;
using AudioOutput = WebAudioOutput;


[[nodiscard]] inline MediaStatus mediaStatus(const Playback& player) noexcept {
    return player.status();
}

[[nodiscard]] inline bool hasError(const Playback& player) noexcept { return player.hasError(); }

[[nodiscard]] inline QString errorText(const Playback& player) { return player.errorText(); }

[[nodiscard]] inline bool isLooping(const Playback& player) noexcept { return player.isLooping(); }

inline void setLooping(Playback& player, bool enabled) { player.setLooping(enabled); }

// The browser keeps its own track list, which the settings menu reads unchanged.
[[nodiscard]] inline std::vector<TrackInfo> trackInfos(const Playback& player, TrackKind kind) {
    return kind == TrackKind::Audio ? player.audioTracks() : player.subtitleTracks();
}

[[nodiscard]] inline AudioOutput* makeAudioOutput(Playback& playback, QObject* parent) {
    return new AudioOutput(&playback, parent);
}

#else

using Playback = QMediaPlayer;
using AudioOutput = QAudioOutput;

/// QAudioOutput takes only a parent; the browser build needs the playback that owns the
/// element, so both platforms go through here.
[[nodiscard]] inline AudioOutput* makeAudioOutput(Playback&, QObject* parent) { return new AudioOutput(parent); }

[[nodiscard]] inline MediaStatus mediaStatus(const Playback& player) noexcept {
    switch (player.mediaStatus()) {
    case QMediaPlayer::NoMedia:
        return MediaStatus::NoMedia;
    case QMediaPlayer::LoadingMedia:
        return MediaStatus::Loading;
    case QMediaPlayer::LoadedMedia:
        return MediaStatus::Loaded;
    case QMediaPlayer::StalledMedia:
        return MediaStatus::Stalled;
    case QMediaPlayer::BufferingMedia:
    case QMediaPlayer::BufferedMedia:
        return MediaStatus::Buffering;
    case QMediaPlayer::EndOfMedia:
        return MediaStatus::EndOfMedia;
    case QMediaPlayer::InvalidMedia:
        return MediaStatus::Invalid;
    }
    return MediaStatus::NoMedia;
}

[[nodiscard]] inline bool hasError(const Playback& player) noexcept {
    return player.error() != QMediaPlayer::NoError;
}

[[nodiscard]] inline QString errorText(const Playback& player) { return player.errorString(); }

[[nodiscard]] inline bool isLooping(const Playback& player) noexcept {
    return player.loops() == QMediaPlayer::Infinite;
}

inline void setLooping(Playback& player, bool enabled) {
    player.setLoops(enabled ? QMediaPlayer::Infinite : QMediaPlayer::Once);
}

[[nodiscard]] inline std::vector<TrackInfo> trackInfos(const Playback& player, TrackKind kind) {
    const QList<QMediaMetaData> list =
        kind == TrackKind::Audio ? player.audioTracks() : player.subtitleTracks();
    std::vector<TrackInfo> infos;
    infos.reserve(static_cast<std::size_t>(list.size()));
    for (const auto& meta : list)
        infos.push_back(TrackInfo{meta.stringValue(QMediaMetaData::Title),
                                  meta.stringValue(QMediaMetaData::Language)});
    return infos;
}

#endif

} // namespace shadcn::detail
