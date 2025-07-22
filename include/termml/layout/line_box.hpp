#ifndef AMT_TERMML_LAYOUT_LINE_BOX_HPP
#define AMT_TERMML_LAYOUT_LINE_BOX_HPP

#include "../core/bounding_box.hpp"

namespace termml::layout {
    struct LineBox {
        std::string_view line{};
        core::BoundingBox bounds{};
    };

    struct LineSpan {
        unsigned start{};
        unsigned size{};

        constexpr auto empty() const noexcept -> bool { return size == 0; }
    };

} // namespace termml::layout

#include <format>

template <>
struct std::formatter<termml::layout::LineBox> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(termml::layout::LineBox const& v, auto& ctx) const {
        auto out = ctx.out();
        std::format_to(out, "LineBox(line: '{}', bounds: {})", v.line, v.bounds);
        return out;
    }
};

template <>
struct std::formatter<termml::layout::LineSpan> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(termml::layout::LineSpan const& v, auto& ctx) const {
        auto out = ctx.out();
        std::format_to(out, "LineSpan(start: {}, size: {})", v.start, v.size);
        return out;
    }
};

#endif // AMT_TERMML_LAYOUT_LINE_BOX_HPP
