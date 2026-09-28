// SPDX-License-Identifier: MIT
#include "playback.hpp"

#include <QTimer>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <emscripten/emscripten.h>

// The browser side. It moves pixels and mirrors values C++ has already decided. It holds
// no playback state, draws no control and never starts playback on its own: a poll reads
// back what the element did and C++ turns that into signals.
namespace {
enum PollIndex {
    PollWidth = 0,
    PollHeight,
    PollReadyState,
    PollError,
    PollPaused,
    PollEnded,
    PollDurationMs,
    PollTimeMs,
    PollCount,
};
// Fast while frames arrive, slow while the element is idle. A media element only decodes
// while it is in the document, so it is parked off-screen rather than removed from the
// tree or hidden with display:none.
constexpr int FrameInterval = 33;
constexpr int IdleInterval = 250;
} // namespace

// Each playback owns one element, so two players on a page do not fight over one source
// and destroying one does not stop the other. The elements are held in a registry and
// addressed by the slot C++ is given here.
EM_JS(int, shadcn_web_media_bridge, (), {
  var registry = globalThis.shadcnWebMedia;
  if (!registry) registry = globalThis.shadcnWebMedia = [];
  var video = document.createElement('video');
  video.crossOrigin = 'anonymous';
  video.playsInline = true;
  video.preload = 'auto';
  video.volume = 1;
  video.style.cssText =
      'position:absolute;left:-100000px;top:0;width:4px;height:4px;pointer-events:none;';
  // A media element only decodes while it is in the document, so it is parked off-screen
  // rather than removed from the tree or hidden with display:none.
  (document.body || document.documentElement).appendChild(video);
  var canvas = document.createElement('canvas');
  var state = {
    video: video,
    canvas: canvas,
    // willReadFrequently keeps the canvas backing store on the CPU, which is the only
    // place getImageData can copy from cheaply.
    context: canvas.getContext('2d', {willReadFrequently: true, alpha: false}),
    dirty: true,
  };
  // requestVideoFrameCallback marks which frames are new so an unchanged frame is not
  // copied again. A browser without it falls back to copying on every poll.
  if (video.requestVideoFrameCallback) {
    var watch = function() {
      state.dirty = true;
      video.requestVideoFrameCallback(watch);
    };
    video.requestVideoFrameCallback(watch);
  }
  registry.push(state);
  return registry.length - 1;
});

EM_JS(void, shadcn_web_media_dispose, (int slot), {
  var registry = globalThis.shadcnWebMedia;
  var state = registry && registry[slot];
  if (!state) return;
  registry[slot] = null;
  state.video.pause();
  state.video.removeAttribute('src');
  state.video.load();
  if (state.video.parentNode) state.video.parentNode.removeChild(state.video);
});

EM_JS(void, shadcn_web_media_load, (int slot, const char* url), {
  var state = globalThis.shadcnWebMedia && globalThis.shadcnWebMedia[slot];
  if (!state) return;
  state.dirty = true;
  var href = url ? UTF8ToString(url) : String();
  if (!href) {
    state.video.removeAttribute('src');
    state.video.load();
    return;
  }
  // The browser owns URL resolution, so a relative name resolves against the page that
  // loaded the gallery rather than against the WebAssembly file system.
  state.video.src = new URL(href, document.baseURI).href;
  state.video.load();
});

EM_JS(void, shadcn_web_media_play, (int slot), {
  var state = globalThis.shadcnWebMedia && globalThis.shadcnWebMedia[slot];
  if (!state) return;
  // A blocked play() rejects; the poll reports the element's own state, so the rejection
  // is swallowed here rather than left as an unhandled promise.
  var attempt = state.video.play();
  if (attempt && attempt.catch) attempt.catch(function() {});
});

EM_JS(void, shadcn_web_media_pause, (int slot), {
  var state = globalThis.shadcnWebMedia && globalThis.shadcnWebMedia[slot];
  if (state) state.video.pause();
});

EM_JS(void, shadcn_web_media_seek, (int slot, double seconds), {
  var state = globalThis.shadcnWebMedia && globalThis.shadcnWebMedia[slot];
  if (!state) return;
  try {
    state.video.currentTime = seconds;
  } catch (error) {
    // The element refuses a seek before metadata arrives. The next poll catches up.
  }
});

EM_JS(void, shadcn_web_media_rate, (int slot, double rate), {
  var state = globalThis.shadcnWebMedia && globalThis.shadcnWebMedia[slot];
  if (state) state.video.playbackRate = rate;
});

EM_JS(void, shadcn_web_media_loop, (int slot, int enabled), {
  var state = globalThis.shadcnWebMedia && globalThis.shadcnWebMedia[slot];
  if (state) state.video.loop = enabled !== 0;
});

EM_JS(void, shadcn_web_media_volume, (int slot, double volume, int muted), {
  var state = globalThis.shadcnWebMedia && globalThis.shadcnWebMedia[slot];
  if (!state) return;
  state.video.volume = Math.max(0, Math.min(1, volume));
  state.video.muted = muted !== 0;
});

// Writes the element's state to info and returns 1 when it also copied a decoded frame
// into the caller's buffer. A poll that copies nothing still reports the state.
//
// Emscripten places an EM_JS body inside the module factory, where HEAP32 and HEAPU8 are
// the live views. They are addressed directly rather than through Module, because Module
// only carries them when the build exports them.
EM_JS(int, shadcn_web_media_poll, (int slot, int pixels, int capacity, int* info), {
  var state = globalThis.shadcnWebMedia && globalThis.shadcnWebMedia[slot];
  if (!state) return 0;
  var video = state.video;
  var at = info >> 2;
  HEAP32[at + 0] = video.videoWidth;
  HEAP32[at + 1] = video.videoHeight;
  HEAP32[at + 2] = video.readyState;
  HEAP32[at + 3] = video.error ? video.error.code : 0;
  HEAP32[at + 4] = video.paused ? 1 : 0;
  HEAP32[at + 5] = video.ended ? 1 : 0;
  var duration = video.duration;
  HEAP32[at + 6] = isFinite(duration) ? Math.round(duration * 1000) : -1;
  HEAP32[at + 7] = Math.round(video.currentTime * 1000);
  if (video.requestVideoFrameCallback && !state.dirty) return 0;
  var width = video.videoWidth;
  var height = video.videoHeight;
  if (video.readyState < 2 || width <= 0 || height <= 0) return 0;
  if (capacity < width * height * 4) return 0;
  if (state.canvas.width !== width || state.canvas.height !== height) {
    state.canvas.width = width;
    state.canvas.height = height;
  }
  state.context.drawImage(video, 0, 0, width, height);
  // getImageData returns RGBA bytes, which is what Format_RGBA8888 expects.
  HEAPU8.set(state.context.getImageData(0, 0, width, height).data, pixels);
  state.dirty = false;
  return 1;
});

namespace shadcn::detail {
namespace {
// The browser cannot reach a file the WebAssembly file system holds, so those schemes
// fail here rather than after a silent load attempt.
bool browserCanFetch(const QUrl& source) {
    const auto scheme = source.scheme();
    return scheme != u"file" && scheme != u"qrc" && scheme != u"qtfile";
}
} // namespace

WebPlayback::WebPlayback(QObject* parent) : QObject(parent), pump_(new QTimer(this)) {
    slot_ = shadcn_web_media_bridge();
    pump_->setInterval(IdleInterval);
    connect(pump_, &QTimer::timeout, this, &WebPlayback::poll);
}

WebPlayback::~WebPlayback() {
    pump_->stop();
    if (slot_ >= 0)
        shadcn_web_media_dispose(slot_);
}

void WebPlayback::setSource(const QUrl& source) {
    if (slot_ >= 0) {
        const auto url =
            source.isEmpty() || !browserCanFetch(source)
                ? QByteArray()
                : source.toString(QUrl::FullyEncoded).toUtf8();
        shadcn_web_media_load(slot_, url.constData());
    }
    source_ = source;
    position_ = 0;
    errorCode_ = source.isEmpty() || browserCanFetch(source) ? NoError : UnsupportedSourceError;
    // Nothing is left to poll once the element is empty, so the timer stops rather than
    // waking every quarter second for the rest of the widget's life.
    if (source_.isEmpty())
        pump_->stop();
    else
        pump_->start();
    poll();
}

void WebPlayback::play() {
    if (source_.isEmpty() || errorCode_ != NoError)
        return;
    shadcn_web_media_play(slot_);
    poll();
}

void WebPlayback::pause() {
    if (slot_ >= 0)
        shadcn_web_media_pause(slot_);
    poll();
}

void WebPlayback::stop() {
    pause();
    setPosition(0);
}

void WebPlayback::setPosition(qint64 milliseconds) {
    if (slot_ < 0 || !seekable_)
        return;
    shadcn_web_media_seek(slot_, static_cast<double>(std::clamp<qint64>(milliseconds, 0, duration_)) / 1000.);
    poll();
}

void WebPlayback::setPlaybackRate(qreal rate) {
    playbackRate_ = rate;
    if (slot_ >= 0)
        shadcn_web_media_rate(slot_, static_cast<double>(rate));
}

void WebPlayback::setLooping(bool enabled) {
    looping_ = enabled;
    if (slot_ >= 0)
        shadcn_web_media_loop(slot_, enabled ? 1 : 0);
}

QString WebPlayback::errorText() const {
    switch (errorCode_) {
    case NoError:
        return {};
    case AbortedError:
        return tr("Playback was stopped before it finished.");
    case NetworkError:
        return tr("A network error interrupted the download.");
    case DecodeError:
        return tr("The browser could not decode this video.");
    case UnsupportedSourceError:
        return tr("The browser cannot reach a local file. Give an http, https or blob URL.");
    default:
        return tr("The browser reported an unknown media error.");
    }
}

void WebPlayback::applyAudio(qreal volume, bool muted) {
    if (slot_ >= 0)
        shadcn_web_media_volume(slot_, static_cast<double>(volume), muted ? 1 : 0);
}

void WebPlayback::poll() {
    if (slot_ < 0)
        return;
    // The browser only copies into a buffer it was told it may fill, so the buffer is
    // sized from the frame size the previous poll reported. Sizing it after a successful
    // copy would never grow it past the first empty one.
    const auto previous = std::size_t(frameWidth_) * std::size_t(frameHeight_) * 4;
    if (previous > pixels_.size())
        pixels_.resize(previous);
    int info[PollCount]{};
    const auto copied = shadcn_web_media_poll(slot_, reinterpret_cast<int>(pixels_.data()),
                                              static_cast<int>(pixels_.size()), info);
    frameWidth_ = info[PollWidth];
    frameHeight_ = info[PollHeight];
    if (copied != 0) {
        const QImage frame(pixels_.data(), frameWidth_, frameHeight_, frameWidth_ * 4,
                           QImage::Format_RGBA8888);
        emit frameAvailable(frame);
    }
    // The element reports itself; C++ owns every value the transport reads.
    const auto readyState = info[PollReadyState];
    const auto reportedError = info[PollError];
    const auto ended = info[PollEnded] != 0;
    // An element with no source reports a negative or non-finite duration, and the
    // transport reads zero there, as QMediaPlayer does.
    const auto duration = std::max(qint64{0}, qint64(info[PollDurationMs]));
    const auto position = std::clamp(qint64(info[PollTimeMs]), qint64{0}, duration);
    const auto playing = info[PollPaused] == 0 && !ended && errorCode_ == NoError;
    const auto seekable = readyState >= 1 && duration > 0;
    auto nextStatus = status_;
    auto nextError = errorCode_;
    if (nextError == NoError && reportedError != 0)
        nextError = reportedError;
    if (nextError != NoError)
        nextStatus = MediaStatus::Invalid;
    else if (source_.isEmpty())
        nextStatus = MediaStatus::NoMedia;
    else if (ended)
        nextStatus = MediaStatus::EndOfMedia;
    else if (readyState >= 3)
        nextStatus = MediaStatus::Loaded;
    else if (readyState == 2)
        nextStatus = MediaStatus::Buffering;
    else
        nextStatus = MediaStatus::Loading;

    const auto previousDuration = duration_;
    const auto previousPosition = position_;
    const auto previousStatus = status_;
    const auto previousError = errorCode_;
    const bool wasPlaying = playing_;
    const bool wasSeekable = seekable_;
    duration_ = duration;
    position_ = position;
    playing_ = playing;
    seekable_ = seekable;
    status_ = nextStatus;
    errorCode_ = nextError;
    pump_->setInterval(playing_ ? FrameInterval : IdleInterval);
    if (duration_ != previousDuration)
        emit durationChanged(duration_);
    if (seekable_ != wasSeekable)
        emit seekableChanged();
    if (playing_ != wasPlaying)
        emit playbackStateChanged();
    if (position_ != previousPosition)
        emit positionChanged(position_);
    if (status_ != previousStatus)
        emit mediaStatusChanged();
    if (errorCode_ != previousError)
        emit errorOccurred();
}

WebAudioOutput::WebAudioOutput(WebPlayback* playback, QObject* parent)
    : QObject(parent), playback_(playback) {
    playback_->applyAudio(volume_, muted_);
}

void WebAudioOutput::setVolume(qreal volume) {
    const auto clamped = std::clamp<qreal>(volume, 0., 1.);
    if (std::abs(volume_ - clamped) < 1e-6)
        return;
    volume_ = clamped;
    playback_->applyAudio(clamped, muted_);
    emit volumeChanged(static_cast<float>(clamped));
}

void WebAudioOutput::setMuted(bool muted) {
    if (muted_ == muted)
        return;
    muted_ = muted;
    playback_->applyAudio(volume_, muted_);
    emit mutedChanged(muted);
}

} // namespace shadcn::detail
