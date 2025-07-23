#ifndef AMT_TERMML_CORE_BOUNDING_BOX_HPP
#define AMT_TERMML_CORE_BOUNDING_BOX_HPP

#include <algorithm>
#include <limits>

namespace termml::core {

    struct BoundingBox {
        int x{};
        int y{};
        int width{};
        int height{};

        constexpr auto min_x() const noexcept -> int { return x; }
        constexpr auto min_y() const noexcept -> int { return y; }
        constexpr auto max_x() const noexcept -> int { return x + width; }
        constexpr auto max_y() const noexcept -> int { return y + height; }

        constexpr auto intersects(BoundingBox const& other) const noexcept -> bool {
            return !(max_x() <= other.min_x() ||  // this box is completely to the left
                     min_x() >= other.max_x() ||  // this box is completely to the right
                     max_y() <= other.min_y() ||  // this box is completely above
                     min_y() >= other.max_y());   // this box is completely below
        }

        static constexpr auto inf() noexcept -> BoundingBox {
            return {
                .x = 0,
                .y = 0,
                .width = std::numeric_limits<int>::max(),
                .height = std::numeric_limits<int>::max()
            };
        }

        constexpr auto in(int x_, int y_) const noexcept -> bool {
            return within_x(x_) && within_y(y_);
        }

        constexpr auto within_y(int y_) const noexcept -> bool {
            return min_y() <= y_ && y_ < max_y();
        }

        constexpr auto within_x(int x_) const noexcept -> bool {
            return min_x() <= x_ && x_ < max_x();
        }

        static constexpr auto from(int min_x, int max_x, int min_y, int max_y) noexcept -> BoundingBox {
            return {
                .x = min_x,
                .y = min_y,
                .width = std::max(max_x - min_x, 0),
                .height = std::min(max_y - min_y, 0)
            };
        }

        constexpr auto pad(int top, int right, int bottom, int left) const noexcept -> BoundingBox {
            auto x_ = x + left;
            auto y_ = y + top;
            auto w = std::max(width - right - left, 0);
            auto h = std::max(height - top - bottom, 0);
            return { x_, y_, w, h };
        }
    };

} // namespace termml::core

#include <format>

template <>
struct std::formatter<termml::core::BoundingBox> {
    bool show_min_max{false};
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            if (*it == 'm') show_min_max = true;
            ++it;
        }
        return it;
    }

    auto format(termml::core::BoundingBox const& v, auto& ctx) const {
        auto out = ctx.out();
        if (show_min_max) {
            std::format_to(out, "BoundingBox(min_x: {}, max_x: {}, min_y: {}, max_y: {})", v.min_x(), v.max_x(), v.min_y(), v.max_y());
        } else {
            std::format_to(out, "BoundingBox(x: {}, y: {}, width: {}, height: {})", v.x, v.y, v.width, v.height);
        }
        return out;
    }
};


#endif // AMT_TERMML_CORE_BOUNDING_BOX_HPP
