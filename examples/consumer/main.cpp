#include <shadcn/core.hpp>
#include <iostream>
int main() {
    const auto theme = shadcn::Theme::neutral(shadcn::ColorMode::Dark);
    const auto fraction = shadcn::progress_fraction(25, 0, 100);
    if (!fraction || *fraction != 0.25 || theme.color(shadcn::Role::Border).a != 0.1) return 1;
    std::cout << "shadcn-cpp " << shadcn::version << ": installed core works\n";
}
