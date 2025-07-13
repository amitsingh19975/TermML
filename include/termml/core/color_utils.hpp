#ifndef AMT_TERMMML_CORE_COLOR_UTILS_HPP
#define AMT_TERMMML_CORE_COLOR_UTILS_HPP

#include <cstdint>
#include <tuple>

namespace termml::core {

    namespace detail {
        constexpr auto hue_to_rgb(float p, float q, float t) noexcept -> float {
            if (t < 0) t += 1;
            if (t > 1) t -= 1;
            if (t * 6 < 1) return p + (q - p) * 6 * t;
            if (t * 2 < 1) return q;
            if (t * 3 < 2) return p + (q - p) * (2.0f/3.0f - t) * 6;
            return p;
        }
    } // namespace detail 

    constexpr auto hsl_to_rgb(float h, float s, float l) noexcept -> std::tuple<std::uint8_t, std::uint8_t, std::uint8_t> {
        using detail::hue_to_rgb;
        auto th = float(h) / 360;
        auto ts = float(s) / 100;
        auto tl = float(l) / 100;

        auto r = float{};
        auto g = float{};
        auto b = float{};

        if (ts <= 0.00001f) {
            r = g = b = tl;
        } else {
            auto q = tl < 0.5f ? tl * (1 + ts) : tl + ts - tl * ts;
            auto p = 2.f * tl - q;
            r = hue_to_rgb(p, q, th + 1.f/3);
            g = hue_to_rgb(p, q, th);
            b = hue_to_rgb(p, q, th - 1.f/3);
        }

        return {
            static_cast<std::uint8_t>(r * 255),
            static_cast<std::uint8_t>(g * 255),
            static_cast<std::uint8_t>(b * 255),
        };
    }

} // namespace termml::core

#endif // AMT_TERMMML_CORE_COLOR_UTILS_HPP
