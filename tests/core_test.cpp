#include <shadcn/core.hpp>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace {
int failures = 0;
int checks = 0;
void expect(bool condition, std::string_view description) {
    ++checks;
    if (!condition) { ++failures; std::cerr << "FAIL: " << description << '\n'; }
}
bool close(double a, double b, double tolerance = 1e-6) {
    return std::abs(a - b) < tolerance;
}
}
int main() {
    using namespace shadcn;
    const auto white = to_srgb({1, 0, 0});
    expect(white && close(white->r, 1) && close(white->g, 1) && close(white->b, 1),
           "OKLCH white becomes sRGB white");
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();
    expect(!to_srgb({nan, 0, 0}), "NaN colour is rejected");
    expect(!to_srgb({0.5, 0, inf}), "infinite hue is rejected");
    expect(!to_srgb({1.1, 0, 0}), "out-of-range lightness is rejected");
    expect(!to_srgb({0.5, -0.1, 0}), "negative chroma is rejected");
    expect(!to_srgb({0.5, 0, 0, 1.1}), "out-of-range alpha is rejected");
    const auto black = to_srgb({0, 0, 0});
    expect(black && black->r == 0 && black->g == 0 && black->b == 0, "OKLCH black");
    const auto red = to_srgb({0.62795536, 0.25768331, 29.233885});
    expect(red && close(red->r, 1, 1e-5) && red->g < 1e-5 && red->b < 1e-5,
           "independent OKLCH sRGB red reference");
    const auto transparent = to_srgb({1, 0, 720, 0.15});
    expect(transparent && transparent->a == 0.15, "alpha is preserved");
    const auto fraction = progress_fraction(25, 0, 100);
    expect(fraction && *fraction == 0.25, "25 of 100 is one quarter");
    expect(!progress_fraction(0, 0, 0), "empty range is rejected");
    expect(!progress_fraction(50, 100, 0), "reversed range is rejected");
    expect(!progress_fraction(-1, 0, 100), "negative out-of-range progress is rejected");
    expect(!progress_fraction(101, 0, 100), "progress above max is rejected");
    expect(!progress_fraction(nan, 0, 100), "NaN progress is rejected");
    const auto huge = std::numeric_limits<double>::max();
    const auto wide = progress_fraction(0, -huge, huge);
    expect(wide && *wide == 0.5, "wide finite range does not overflow");
    const auto linear = CubicBezier::make(0, 0, 1, 1);
    expect(linear && close(linear->sample(0.37), 0.37), "linear timing identity");
    expect(!CubicBezier::make(-0.1, 0, 1, 1), "non-monotonic x control is rejected");
    expect(!CubicBezier::make(0, nan, 1, 1), "NaN timing control is rejected");
    expect(transition_easing(0) == 0 && transition_easing(1) == 1, "exact timing endpoints");
    expect(close(transition_easing(0.5), 0.77556131), "Tailwind default transition worked reference");
    expect(transition_easing(-1) == 0 && transition_easing(2) == 1, "timing input is clamped");
    expect(transition_easing(nan) == 0, "non-finite timing input is safe");
    expect(close(pulse_opacity(0), 1), "pulse starts opaque");
    expect(close(pulse_opacity(1), 0.5), "pulse is half opaque after one second");
    expect(close(pulse_opacity(2), 1), "pulse repeats every two seconds");
    expect(close(pulse_opacity(0.5), 0.75), "pulse first half uses symmetric CSS easing");
    const auto light = Theme::neutral();
    const auto dark = Theme::neutral(ColorMode::Dark);
    expect(light.mode() == ColorMode::Light && dark.mode() == ColorMode::Dark, "theme mode");
    expect(close(light.color(Role::Background).r, 1), "light background is white");
    expect(close(dark.color(Role::Border).a, 0.1), "dark border has 10 percent alpha");
    expect(close(dark.color(Role::Input).a, 0.15), "dark input has 15 percent alpha");
    expect(light.radius() == 10, "0.625 rem is ten logical pixels");
    auto custom = light;
    const auto old = custom.color(Role::Primary);
    expect(!custom.set(Role::Primary, {nan, 0, 0, 1}), "custom theme rejects NaN");
    expect(custom.color(Role::Primary) == old, "invalid update does not mutate a theme");
    expect(custom.set(Role::Primary, {0.8, 0.1, 0.2, 1}).has_value(), "custom primary accepted");
    expect(!custom.set(Role::Count, {0, 0, 0, 1}), "invalid role rejected");
    expect(!custom.setRadius(inf) && !custom.setRadius(-1), "invalid radii rejected");
    expect(custom.setRadius(12).has_value() && custom.radius() == 12, "radius update accepted");
    expect(button_metrics(ButtonSize::Default).height == 36, "upstream default button h-9");
    expect(button_metrics(ButtonSize::Xs).height == 24, "upstream xs button h-6");
    expect(button_metrics(ButtonSize::Sm).iconPadding == 10, "upstream sm icon padding");
    expect(button_metrics(ButtonSize::IconLg).iconOnly, "icon-lg is square");
    double previous = 0;
    for (int i = 0; i <= 1000; ++i) {
        const auto value = transition_easing(static_cast<double>(i) / 1000);
        expect(value >= previous && value >= 0 && value <= 1, "timing is bounded and monotonic");
        previous = value;
    }
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
