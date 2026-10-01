// SPDX-License-Identifier: MIT
#include "playback.hpp"

#include <QTimer>
#include <QPointer>
#include <limits>
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
    PollSeekable,
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
    disposed: false,
    callback: 0,
  };
  // requestVideoFrameCallback marks which frames are new so an unchanged frame is not
  // copied again. A browser without it falls back to copying on every poll.
  if (video.requestVideoFrameCallback) {
    var watch = function() {
      if (state.disposed) return;
      state.dirty = true;
      state.callback = video.requestVideoFrameCallback(watch);
    };
    state.callback = video.requestVideoFrameCallback(watch);
  }
  if (!registry.freeSlots) registry.freeSlots = [];
  var slot = registry.freeSlots.length ? registry.freeSlots.pop() : registry.length;
  registry[slot] = state;
  return slot;
});

EM_JS(void, shadcn_web_media_dispose, (int slot), {
  var registry = globalThis.shadcnWebMedia;
  var state = registry && registry[slot];
  if (!state) return;
  state.disposed = true;
  if (state.callback && state.video.cancelVideoFrameCallback)
    state.video.cancelVideoFrameCallback(state.callback);
  registry[slot] = null;
  registry.freeSlots.push(slot);
  state.canvas.width = state.canvas.height = 0;
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
    state.canvas.width = state.canvas.height = 0;
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
EM_JS(int, shadcn_web_media_poll, (int slot, int pixels, int capacity, int* info, double* timing), {
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
  HEAP32[at + 6] = video.seekable.length > 0 ? 1 : 0;
  var times = timing >> 3;
  HEAPF64[times] = isFinite(duration) ? duration * 1000 : 0;
  HEAPF64[times + 1] = video.currentTime * 1000;
  if (video.requestVideoFrameCallback && !state.dirty) return 0;
  var width = video.videoWidth;
  var height = video.videoHeight;
  if (video.readyState < 2 || width <= 0 || height <= 0) return 0;
  if (capacity < width * height * 4) return 0;
  if (state.canvas.width !== width || state.canvas.height !== height) {
    state.canvas.width = width;
    state.canvas.height = height;
  }
  try {
    if (!state.context) return 0;
    state.context.drawImage(video, 0, 0, width, height);
    // A cross-origin source can decode but forbid canvas access. Report the error.
    HEAPU8.set(state.context.getImageData(0, 0, width, height).data, pixels);
  } catch (error) {
    HEAP32[at + 3] = 3;
    return 0;
  }
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
    frameWidth_ = frameHeight_ = 0;
    if (source.isEmpty()) std::vector<uint8_t>{}.swap(pixels_);
    else pixels_.clear();
    ++sourceRevision_;
    const QPointer<WebPlayback> alive(this);
    const auto revision = sourceRevision_;
    emit frameAvailable(QImage{});
    if (!alive || sourceRevision_ != revision) return;
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
    const QPointer<WebPlayback> alive(this);
    const auto revision = sourceRevision_;
    pause();
    if (alive && sourceRevision_ == revision) setPosition(0);
}

void WebPlayback::setPosition(qint64 milliseconds) {
    if (slot_ < 0 || !seekable_)
        return;
    shadcn_web_media_seek(slot_, static_cast<double>(std::clamp<qint64>(milliseconds, 0, duration_)) / 1000.);
    poll();
}

void WebPlayback::setPlaybackRate(qreal rate) {
    if (!std::isfinite(rate) || rate <= 0) return;
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
    const QPointer<WebPlayback> alive(this);
    const auto revision = sourceRevision_;
    const auto unchanged = [&] { return alive && sourceRevision_ == revision; };
    // A frame must fit both the Emscripten buffer size and QImage's row stride.
    const auto previous = std::uint64_t(std::max(0, frameWidth_)) *
                          std::uint64_t(std::max(0, frameHeight_)) * 4;
    const bool frameFits = previous <= std::size_t(std::numeric_limits<int>::max());
    if (frameFits && previous > pixels_.size()) pixels_.resize(static_cast<std::size_t>(previous));
    int info[PollCount]{};
    double timing[2]{};
    const auto copied = shadcn_web_media_poll(slot_, reinterpret_cast<int>(pixels_.data()),
                                            static_cast<int>(pixels_.size()), info, timing);
    frameWidth_ = info[PollWidth];
    frameHeight_ = info[PollHeight];
    const auto milliseconds = [](double value) -> qint64 {
        // JavaScript integers are exact through 2^53-1, well below qint64's limit.
        return std::isfinite(value) && value > 0
                   ? static_cast<qint64>(std::min(std::round(value), 9007199254740991.)) : 0;
    };
    const auto duration = source_.isEmpty() ? qint64{0} : milliseconds(timing[0]);
    const auto position = std::clamp(milliseconds(timing[1]), qint64{0}, duration);
    const auto nextError = source_.isEmpty() ? NoError :
        !browserCanFetch(source_) ? UnsupportedSourceError :
        !frameFits ? DecodeError : info[PollError];
    const bool ended = info[PollEnded] != 0;
    const bool playing = info[PollPaused] == 0 && !ended && nextError == NoError;
    const bool seekable = info[PollSeekable] != 0 && duration > 0 && nextError == NoError;
    const auto nextStatus = nextError != NoError ? MediaStatus::Invalid :
        source_.isEmpty() ? MediaStatus::NoMedia : ended ? MediaStatus::EndOfMedia :
        info[PollReadyState] >= 3 ? MediaStatus::Loaded :
        info[PollReadyState] == 2 ? MediaStatus::Buffering : MediaStatus::Loading;
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
    if (source_.isEmpty() || nextError != NoError) pump_->stop();
    if (copied != 0) {
        const QImage frame(pixels_.data(), frameWidth_, frameHeight_, frameWidth_ * 4,
                           QImage::Format_RGBA8888);
        emit frameAvailable(frame.copy());
        if (!unchanged()) return;
    }
    if (duration_ != previousDuration) {
        emit durationChanged(duration_);
        if (!unchanged()) return;
    }
    if (seekable_ != wasSeekable) {
        emit seekableChanged();
        if (!unchanged()) return;
    }
    if (playing_ != wasPlaying) {
        emit playbackStateChanged();
        if (!unchanged()) return;
    }
    if (position_ != previousPosition) {
        emit positionChanged(position_);
        if (!unchanged()) return;
    }
    if (status_ != previousStatus) {
        emit mediaStatusChanged();
        if (!unchanged()) return;
    }
    if (errorCode_ != previousError && errorCode_ != NoError) emit errorOccurred();
}

WebAudioOutput::WebAudioOutput(WebPlayback* playback, QObject* parent)
    : QObject(parent), playback_(playback) {
    if (playback_) playback_->applyAudio(volume_, muted_);
}

void WebAudioOutput::setVolume(qreal volume) {
    if (!std::isfinite(volume)) return;
    const auto clamped = std::clamp<qreal>(volume, 0., 1.);
    if (std::abs(volume_ - clamped) < 1e-6)
        return;
    volume_ = clamped;
    if (playback_) playback_->applyAudio(clamped, muted_);
    emit volumeChanged(static_cast<float>(clamped));
}

void WebAudioOutput::setMuted(bool muted) {
    if (muted_ == muted)
        return;
    muted_ = muted;
    if (playback_) playback_->applyAudio(volume_, muted_);
    emit mutedChanged(muted);
}

} // namespace shadcn::detail
