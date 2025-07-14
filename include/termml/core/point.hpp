#ifndef AMT_TERMML_CORE_POINT_HPP
#define AMT_TERMML_CORE_POINT_HPP

#include <algorithm>
namespace termml::core {

    struct Point {
        int x{};
        int y{};

        constexpr auto clamp_x(int min, int max) const noexcept -> Point {
            return { .x = std::clamp(x, min, max), .y = y };
        }

        constexpr auto clamp_y(int min, int max) const noexcept -> Point {
            return { .x = x, .y = std::clamp(y, min, max) };
        }
    };

} // namespace termml::core

#include <format>

template <>
struct std::formatter<termml::core::Point> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(termml::core::Point const& v, auto& ctx) const {
        auto out = ctx.out();
        std::format_to(out, "Point(x: {}, y: {})", v.x, v.y);
        return out;
    }
};


#endif // AMT_TERMML_CORE_POINT_HPP
