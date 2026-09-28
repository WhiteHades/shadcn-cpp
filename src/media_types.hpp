// SPDX-License-Identifier: MIT
#pragma once

// The vocabulary the video player's playback seam is written in. Kept apart from
// playback.hpp because the browser backend is moc'd on its own, and a moc'd header can
// only include headers that stand alone.

#include <QString>

#include <vector>

namespace shadcn::detail {

/// The playback state the transport needs to report. QMediaPlayer::MediaStatus on a
/// native build, the HTMLMediaElement readyState mapped by WebPlayback in the browser.
enum class MediaStatus { NoMedia, Loading, Loaded, Stalled, Buffering, EndOfMedia, Invalid };

enum class TrackKind { Audio, Subtitles };

/// One entry of a track menu. QMediaPlayer reports QMediaMetaData, which the native
/// helper narrows to the two fields the settings menu reads.
struct TrackInfo {
    QString title;
    QString language;
};

} // namespace shadcn::detail
