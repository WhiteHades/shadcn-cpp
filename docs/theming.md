# Themes and motion

`Theme::neutral` returns an owned light or dark theme. The initial values come from the pinned upstream neutral theme. Theme lookup is a fixed array access; it does not parse a stylesheet during painting.

```cpp
auto theme = shadcn::Theme::neutral(shadcn::ColorMode::Dark);
const auto changed = theme.set(shadcn::Role::Primary, {0.8, 0.12, 0.18, 1.0});
if (!changed) {
    // The previous colour is unchanged.
}
shadcn::install(app, theme, shadcn::MotionPolicy::Reduced);
```

The colour setter rejects NaN, infinity and channels outside zero to one. Theme creation uses OKLCH values from the source. The current conversion clips outside the sRGB gamut sRGB channels. Browser CSS can use a different gamut mapping method, so colour parity remains unverified.

The reference rem is 16 logical pixels. The neutral radius is 10 pixels. Button dimensions come from their own source variants. The default application font size is 14 logical pixels and the existing font family is retained. Pass a zero or negative final argument to `install` to preserve the application font size. Fonts are not bundled. Visual comparisons must use the same font on both sides.

`install` changes the QApplication style and palette. It must run on the GUI thread. Defer a style replacement until after the current event handler when changing a theme interactively; the gallery demonstrates this with a timer with zero delay.

Full motion uses a 150 ms default timing curve for the draft button, switch and progress transitions. Skeleton uses a pulse lasting two seconds. Reduced motion disables these transitions and the pulse. Hidden skeletons stop animating. The current API requires an explicit motion policy; automatic operating system preference detection is planned.

Only the neutral theme and New York v4 profile are implemented as drafts. Chart and sidebar colour roles do not mean that Chart or Sidebar components exist. Arbitrary browser CSS, Tailwind class strings and alternate shadcn profiles are outside the current implementation.
