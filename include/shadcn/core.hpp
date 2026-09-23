// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <cstddef>
#include <expected>

namespace shadcn {

/// Library version.
inline constexpr auto version = "0.1.0";

/// Button appearances.
enum class Variant { Default, Destructive, Outline, Secondary, Ghost, Link };
/// Button sizes, including all four sizes for icons.
enum class ButtonSize { Default, Xs, Sm, Lg, Icon, IconXs, IconSm, IconLg };
enum class ColorMode { Light, Dark };
enum class MotionPolicy { Full, Reduced };

/// Dimensions use logical pixels, with 16 pixels per CSS rem.
struct ButtonMetrics {
    double height;
    double padding;
    double iconPadding;
    double gap;
    double iconSize;
    double fontSize;
    bool iconOnly;
};

[[nodiscard]] constexpr ButtonMetrics button_metrics(ButtonSize size) noexcept {
    switch (size) {
    case ButtonSize::Xs: return {24, 8, 6, 4, 12, 12, false};
    case ButtonSize::Sm: return {28, 10, 6, 4, 14, 12.8, false};
    case ButtonSize::Lg: return {36, 10, 8, 6, 16, 14, false};
    case ButtonSize::Icon: return {32, 0, 0, 0, 16, 14, true};
    case ButtonSize::IconXs: return {24, 0, 0, 0, 12, 12, true};
    case ButtonSize::IconSm: return {28, 0, 0, 0, 16, 14, true};
    case ButtonSize::IconLg: return {36, 0, 0, 0, 16, 14, true};
    case ButtonSize::Default: return {32, 10, 8, 6, 16, 14, false};
    }
    return {32, 10, 8, 6, 16, 14, false};
}

struct Rgba {
    double r = 0;
    double g = 0;
    double b = 0;
    double a = 1;
    constexpr bool operator==(const Rgba&) const = default;
};
/// Lightness, chroma and alpha use [0, 1] in this implementation. Hue is in degrees.
struct Oklch {
    double lightness;
    double chroma;
    double hue;
    double alpha = 1;
};
enum class ValueError { NonFinite, OutOfRange, EmptyRange };

/// Convert OKLCH to sRGB and clip channels outside the gamut. CSS gamut mapping is not implemented.
[[nodiscard]] std::expected<Rgba, ValueError> to_srgb(Oklch color) noexcept;
/// Reject NaN, infinity, reversed ranges and values outside the range.
[[nodiscard]] std::expected<double, ValueError>
progress_fraction(double value, double minimum, double maximum) noexcept;
/// Evaluate the CSS `cubic-bezier` timing function. x1 and x2 must be in [0, 1].
class CubicBezier {
public:
    [[nodiscard]] static std::expected<CubicBezier, ValueError>
    make(double x1, double y1, double x2, double y2) noexcept;
    /// Clamp finite input to [0, 1]. NaN and infinity return 0.
    [[nodiscard]] double sample(double progress) const noexcept;
private:
    CubicBezier(double x1, double y1, double x2, double y2) noexcept;
    double x1_, y1_, x2_, y2_;
};
/// Tailwind's default transition curve, `cubic-bezier(0.4, 0, 0.2, 1)`.
[[nodiscard]] double transition_easing(double progress) noexcept;
/// Tailwind pulse opacity. One cycle is 2 seconds, with opacity 0.5 halfway.
[[nodiscard]] double pulse_opacity(double seconds) noexcept;

/// Semantic roles from shadcn's neutral theme, including chart and sidebar roles.
enum class Role : std::size_t {
    Background, Foreground, Card, CardForeground, Popover, PopoverForeground,
    Primary, PrimaryForeground, Secondary, SecondaryForeground,
    Muted, MutedForeground, Accent, AccentForeground, Destructive,
    Border, Input, Ring, Chart1, Chart2, Chart3, Chart4, Chart5,
    Sidebar, SidebarForeground, SidebarPrimary, SidebarPrimaryForeground,
    SidebarAccent, SidebarAccentForeground, SidebarBorder, SidebarRing, Count
};

/// A theme that owns its values. Reading a colour is O(1) and allocates no memory.
class Theme {
public:
    [[nodiscard]] static Theme neutral(ColorMode mode = ColorMode::Light);
    /// Invalid enum values return the background colour.
    [[nodiscard]] Rgba color(Role role) const noexcept;
    [[nodiscard]] ColorMode mode() const noexcept { return mode_; }
    /// Invalid colours leave the previous colour unchanged.
    [[nodiscard]] std::expected<void, ValueError> set(Role role, Rgba color) noexcept;
    [[nodiscard]] double radius() const noexcept { return radius_; }
    [[nodiscard]] std::expected<void, ValueError> setRadius(double pixels) noexcept;
private:
    Theme() = default;
    ColorMode mode_ = ColorMode::Light;
    std::array<Rgba, static_cast<std::size_t>(Role::Count)> colors_{};
    double radius_ = 10;
};

} // namespace shadcn
