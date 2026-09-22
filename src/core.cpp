// SPDX-License-Identifier: MIT
#include <shadcn/core.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace shadcn {
namespace {
bool unit(double value) noexcept { return value >= 0 && value <= 1; }
double encode(double value) noexcept {
    value = std::clamp(value, 0.0, 1.0);
    return value <= 0.0031308 ? 12.92 * value
                           : 1.055 * std::pow(value, 1.0 / 2.4) - 0.055;
}
double bezier(double t, double first, double second) noexcept {
    const auto other = 1 - t;
    return 3 * other * other * t * first + 3 * other * t * t * second + t * t * t;
}
} // namespace

std::expected<Rgba, ValueError> to_srgb(Oklch color) noexcept {
    if (!std::isfinite(color.lightness) || !std::isfinite(color.chroma) ||
        !std::isfinite(color.hue) || !std::isfinite(color.alpha)) {
        return std::unexpected(ValueError::NonFinite);
    }
    // A bounded chroma also prevents overflow in the matrix arithmetic.
    if (!unit(color.lightness) || color.chroma < 0 || color.chroma > 1 || !unit(color.alpha)) {
        return std::unexpected(ValueError::OutOfRange);
    }
    const auto radians = std::remainder(color.hue, 360.0) * std::numbers::pi / 180;
    const auto a = color.chroma * std::cos(radians);
    const auto b = color.chroma * std::sin(radians);
    const auto l = color.lightness + 0.3963377774 * a + 0.2158037573 * b;
    const auto m = color.lightness - 0.1055613458 * a - 0.0638541728 * b;
    const auto s = color.lightness - 0.0894841775 * a - 1.2914855480 * b;
    const auto l3 = l * l * l;
    const auto m3 = m * m * m;
    const auto s3 = s * s * s;
    return Rgba{
        encode(4.0767416621 * l3 - 3.3077115913 * m3 + 0.2309699292 * s3),
        encode(-1.2684380046 * l3 + 2.6097574011 * m3 - 0.3413193965 * s3),
        encode(-0.0041960863 * l3 - 0.7034186147 * m3 + 1.7076147010 * s3),
        color.alpha};
}

std::expected<double, ValueError>
progress_fraction(double value, double minimum, double maximum) noexcept {
    if (!std::isfinite(value) || !std::isfinite(minimum) || !std::isfinite(maximum))
        return std::unexpected(ValueError::NonFinite);
    if (maximum <= minimum) return std::unexpected(ValueError::EmptyRange);
    if (value < minimum || value > maximum) return std::unexpected(ValueError::OutOfRange);
    const auto width = maximum - minimum;
    if (std::isfinite(width)) return (value - minimum) / width;
    const auto scale = std::max(std::abs(minimum), std::abs(maximum));
    return (value / scale - minimum / scale) / (maximum / scale - minimum / scale);
}

CubicBezier::CubicBezier(double x1, double y1, double x2, double y2) noexcept
    : x1_(x1), y1_(y1), x2_(x2), y2_(y2) {}
std::expected<CubicBezier, ValueError>
CubicBezier::make(double x1, double y1, double x2, double y2) noexcept {
    if (!std::isfinite(x1) || !std::isfinite(y1) || !std::isfinite(x2) || !std::isfinite(y2))
        return std::unexpected(ValueError::NonFinite);
    if (!unit(x1) || !unit(x2)) return std::unexpected(ValueError::OutOfRange);
    return CubicBezier{x1, y1, x2, y2};
}
double CubicBezier::sample(double progress) const noexcept {
    if (!std::isfinite(progress)) return 0;
    if (progress <= 0) return 0;
    if (progress >= 1) return 1;
    // Invert x(t) before evaluating y(t). Sampling y(progress) is incorrect.
    // A fixed iteration count bounds work and handles flat endpoint derivatives.
    double low = 0, high = 1;
    for (int i = 0; i < 32; ++i) {
        const auto middle = (low + high) / 2;
        if (bezier(middle, x1_, x2_) < progress) low = middle;
        else high = middle;
    }
    return bezier((low + high) / 2, y1_, y2_);
}
double transition_easing(double progress) noexcept {
    static const auto curve = CubicBezier::make(0.4, 0, 0.2, 1).value();
    return curve.sample(progress);
}
double pulse_opacity(double seconds) noexcept {
    if (!std::isfinite(seconds) || seconds < 0) return 1;
    static const auto curve = CubicBezier::make(0.4, 0, 0.6, 1).value();
    const auto phase = std::fmod(seconds, 2.0);
    return phase < 1 ? 1 - 0.5 * curve.sample(phase) : 0.5 + 0.5 * curve.sample(phase - 1);
}

Theme Theme::neutral(ColorMode mode) {
    // Source: shadcn-ui/ui @ 98a1fe6, apps/v4/registry/themes.ts, neutral.
    Theme theme;
    theme.mode_ = mode;
    constexpr std::array<Oklch, 31> light{{
        {1,0,0}, {.145,0,0}, {1,0,0}, {.145,0,0}, {1,0,0}, {.145,0,0},
        {.205,0,0}, {.985,0,0}, {.97,0,0}, {.205,0,0},
        {.97,0,0}, {.556,0,0}, {.97,0,0}, {.205,0,0}, {.577,.245,27.325},
        {.922,0,0}, {.922,0,0}, {.708,0,0},
        {.87,0,0}, {.556,0,0}, {.439,0,0}, {.371,0,0}, {.269,0,0},
        {.985,0,0}, {.145,0,0}, {.205,0,0}, {.985,0,0},
        {.97,0,0}, {.205,0,0}, {.922,0,0}, {.708,0,0}
    }};
    constexpr std::array<Oklch, 31> dark{{
        {.145,0,0}, {.985,0,0}, {.205,0,0}, {.985,0,0}, {.205,0,0}, {.985,0,0},
        {.922,0,0}, {.205,0,0}, {.269,0,0}, {.985,0,0},
        {.269,0,0}, {.708,0,0}, {.269,0,0}, {.985,0,0}, {.704,.191,22.216},
        {1,0,0,.1}, {1,0,0,.15}, {.556,0,0},
        {.87,0,0}, {.556,0,0}, {.439,0,0}, {.371,0,0}, {.269,0,0},
        {.205,0,0}, {.985,0,0}, {.488,.243,264.376}, {.985,0,0},
        {.269,0,0}, {.985,0,0}, {1,0,0,.1}, {.556,0,0}
    }};
    static_assert(light.size() == static_cast<std::size_t>(Role::Count));
    static_assert(dark.size() == static_cast<std::size_t>(Role::Count));
    const auto& source = mode == ColorMode::Light ? light : dark;
    for (std::size_t i = 0; i < source.size(); ++i) theme.colors_[i] = to_srgb(source[i]).value();
    return theme;
}
Rgba Theme::color(Role role) const noexcept {
    const auto index = static_cast<std::size_t>(role);
    return colors_[index < colors_.size() ? index : 0];
}
std::expected<void, ValueError> Theme::set(Role role, Rgba color) noexcept {
    if (!std::isfinite(color.r) || !std::isfinite(color.g) ||
        !std::isfinite(color.b) || !std::isfinite(color.a))
        return std::unexpected(ValueError::NonFinite);
    const auto index = static_cast<std::size_t>(role);
    if (index >= colors_.size() || !unit(color.r) || !unit(color.g) || !unit(color.b) || !unit(color.a))
        return std::unexpected(ValueError::OutOfRange);
    colors_[index] = color;
    return {};
}
std::expected<void, ValueError> Theme::setRadius(double pixels) noexcept {
    if (!std::isfinite(pixels)) return std::unexpected(ValueError::NonFinite);
    if (pixels < 0) return std::unexpected(ValueError::OutOfRange);
    radius_ = pixels;
    return {};
}
} // namespace shadcn
