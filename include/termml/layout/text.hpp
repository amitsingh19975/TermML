#ifndef AMT_TERMML_TEXT_HPP
#define AMT_TERMML_TEXT_HPP

#include "../css/style.hpp"
#include "../core/bounding_box.hpp"
#include "../core/point.hpp"
#include "../core/utf8.hpp"
#include "line_box.hpp"
#include <cassert>
#include <cctype>
#include <vector>

namespace termml::layout {

    struct TextRenderResult {
        core::BoundingBox container;
        std::size_t text_rendered{};
        LineSpan span{};
    };

    namespace detail {
        constexpr auto find_word(std::string_view text, std::size_t pos = 0) noexcept -> std::size_t {
            for (auto i = pos; i < text.size();) {
                auto len = core::utf8::get_length(text[i]);
                if (std::isspace(text[i])) return i;
                i += len;
            }
            return text.size();
        }
    } // namespace detail

    struct TextLayouter {
        std::string_view text;
        // Scroll container
        core::BoundingBox container{core::BoundingBox::inf()};
        // position within the scroll container
        core::Point start_position{};

        // Message content width; ignore padding and margin
        constexpr auto measure_width() const noexcept -> int {
            std::size_t width{};
            auto start = std::size_t{};
            auto it = std::min(text.find('\n'), text.size());
            while (it < text.size()) {
                auto txt = text.substr(start, it);
                width = std::max<std::size_t>(width, core::utf8::calculate_size(txt));
                start = it + 1;
                it = std::min(text.find('\n', start), text.size());
            }

            auto txt = text.substr(start);
            width = std::max<std::size_t>(width, core::utf8::calculate_size(txt));

            return static_cast<int>(width);
        }

        auto operator()(
            std::vector<layout::LineBox>& lines,
            std::size_t previous_text,
            css::Style const& style
        ) -> TextRenderResult {
            //  |---------------------Scroll Container-------------------|
            //  |               |--------ViewBox---------|               |
            //  |               |                        |               |
            //  |               |    P                   |               |
            //  |               |                        |               |
            //  |               |                        |               |
            //  |               |                        |               |
            //  |               |------------------------|               |
            //  |                                                        |
            //  |--------------------------------------------------------|

            if (container.width == 0 || container.height == 0) {
                return {};
            }

            auto width = style.content_width();
            auto box = container;
            box.width = 0;
            box.height = 0;
            if (width == 0) return { .container = box };

            auto dx = start_position.x - container.x;
            auto dy = start_position.y - container.y;
            if (dx < 0 || dy < 0) return { .container = box };

            auto x = container.x + dx;
            auto y = container.y + dy;

            auto len = static_cast<int>(core::utf8::calculate_size(text));

            { // check if it's a continuation of the sentence.
                if (lines.size() == previous_text + 1) {
                    auto& line = lines[previous_text];
                    if (!(line.line.empty() || line.line == " ")) {
                        if (line.bounds.max_x() == x && line.bounds.min_y() == y) {
                            if (line.bounds.max_x() + len >= container.max_x()) {
                                line.bounds.x = container.min_x();
                                line.bounds.y += 1;
                                x = line.bounds.max_x();
                                y = line.bounds.y;
                            }
                        }
                    }
                }
            }

            auto line_start = static_cast<unsigned>(lines.size());

            if (x + len < container.max_x()) {
                box.height = 1;
                auto span = LineSpan{ line_start, 1u };
                lines.push_back(LineBox{
                    .line = text,
                    .bounds = { x, y, len, 1 }
                });
                auto w = static_cast<int>(core::utf8::calculate_size(text));
                box.width = w;
                box.x = x;
                box.y = y;
                start_position = { box.max_x(), y };
                return { .container = box, .text_rendered = text.size(), .span = span };
            }

            box.x = x;
            box.y = y;
            box.width = std::min(
                width,
                container.width
            );

            auto start = std::size_t{};
            auto start_y = y;
            auto max_x = x;
            do {
                if (std::isspace(text[start])) {
                    if (x + 1 >= container.max_x()) {
                        ++y;
                        x = container.min_x();
                        dx = 0, dy = 0;
                        if (y >= container.max_y()) break;
                    }
                    auto render_whitespace = (
                        style.whitespace == css::Whitespace::Pre ||
                        style.whitespace == css::Whitespace::PreWrap ||
                        x != container.min_x()
                    );
                    if (render_whitespace) {
                        auto x_inc = 1;
                        if (text[start] == '\n') {
                            x = container.min_x();
                            x_inc = 0;
                            ++y;
                            dx = 0, dy = 0;
                        }
                        x += x_inc;
                        max_x = std::max(max_x, x);
                    }
                    start += 1;
                }
                auto pos = detail::find_word(text, start);
                auto txt = text.substr(start, pos - start);
                auto sz = static_cast<int>(core::utf8::calculate_size(txt));

                if (x - dx + sz > container.max_x()) {
                    if (x != container.min_x()) {
                        ++y;
                        x = container.min_x();
                        dx = 0;
                        dy = 0;
                        if (y >= container.max_y()) break;
                    }
                }

                bool rendered = false;
                if (style.overflow_wrap == css::OverflowWrap::BreakWord) {
                    if (x - dx + sz > container.max_x()) {
                        auto last_text_end = std::size_t{};
                        auto last_x_start = x;
                        auto w = 0;
                        for (auto i = std::size_t{}; i < txt.size(); ++x) {
                            auto l = core::utf8::get_length(txt[i]);
                            max_x = std::max(max_x, x);
                            if (x + static_cast<int>(l) > container.max_x()) {
                                lines.push_back({
                                    .line = txt.substr(last_text_end, i),
                                    .bounds = { last_x_start, y, w, 1 }
                                });
                                ++y;
                                x = container.min_x();
                                last_x_start = x;
                                last_text_end = i;
                                w = 0;
                                dx = 0;
                                dy = 0;

                                if (y >= container.max_y()) break;
                            }

                            assert(i + l <= txt.size());
                            i += l;
                            ++w;
                        }
                        if (last_text_end != txt.size()) {
                            lines.push_back({
                                .line = txt.substr(last_text_end),
                                .bounds = { last_x_start, y, w, 1 }
                            });
                        }
                        rendered = true;
                    }
                }

                if (!rendered) {
                    lines.push_back(LineBox{
                        .line = txt,
                        .bounds = { x, y, sz, 1 }
                    });
                    x += sz;
                }

                max_x = std::max(max_x, x);
                start = pos;
            } while (start < text.size());

            box.height = y - start_y + 1;
            start_position = {
                .x = x,
                .y = y
            };

            box.width = max_x - box.min_x();

            auto size = static_cast<unsigned>(lines.size()) - line_start;
            auto span = LineSpan{ line_start, size };
            return { .container = box, .text_rendered = text.size(), .span = span };
        };
    };

} // namespace termml::layout

#endif // AMT_TERMML_TEXT_HPP
