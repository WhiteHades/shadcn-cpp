# Media assets

## shadow-sample.mp4

A six-second, 320×180, 24 fps sample the documentation video player plays in the browser.

It is generated rather than recorded, so it carries no third-party rights and nothing to
attribute. Regenerate it with the same command when the content needs to change:

```sh
ffmpeg -y \
  -f lavfi -i "testsrc2=size=320x180:rate=24:duration=6" \
  -f lavfi -i "sine=frequency=330:sample_rate=44100:duration=6" \
  -vf "drawtext=fontfile=assets/fonts/Geist.ttf:text='shadcn-cpp  %{pts\:hms}':x=8:y=h-th-6:fontsize=15:fontcolor=white:box=1:boxcolor=black@0.6:boxborderw=5" \
  -c:v libx264 -profile:v baseline -level 3.1 -pix_fmt yuv420p -crf 32 -g 48 -keyint_min 48 -sc_threshold 0 \
  -c:a aac -profile:a aac_low -b:a 48k -ac 1 -ar 44100 \
  -movflags +faststart assets/media/shadow-sample.mp4
```

`testsrc2` supplies a moving pattern and a burnt-in frame counter, so seeking and playback
speed are visible in a screenshot. The overlay carries the elapsed time for the same
reason.

H.264 in MP4 with AAC audio is chosen because it is the combination every current browser
decodes, which matters here: the WebAssembly build has no Qt Multimedia, so the browser
picks the decoder and the application cannot ask for a different one.

`tools/build-live-previews.sh` copies this file beside the WebAssembly build, because the
browser resolves a relative source against the page that loaded the module rather than
against the WebAssembly file system.
