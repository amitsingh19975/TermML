#ifndef AMT_TERMML_TEXT_HPP
#define AMT_TERMML_TEXT_HPP

#include "style.hpp"
#include "core/bounding_box.hpp"
#include "core/point.hpp"
#include "core/device.hpp"
#include "core/utf8.hpp"
#include <cctype>

namespace termml::text {

    struct TextRenderResult {
        core::BoundingBox container;
        std::size_t text_rendered{};
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

    struct TextRenderer {
        std::string_view text;
        // Scroll container
        core::BoundingBox container{core::BoundingBox::inf()};
        // position within the scroll container
        core::Point start_offset{};

        // next word length for word wrapping if next char after
        // the end of this text is not whitespace or breakable.
        std::size_t next_word_length{};

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

        constexpr auto measure_height(style::Style const& style) noexcept -> int {
            auto null = core::NullScreen(container.width, container.height);
            auto d = core::Device(&null);
            return render(d, style).container.height;
        }

        template <core::detail::IsScreen T>
        auto render(
            core::Device<T>& device,
            style::Style const& style
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

            auto dx = start_offset.x - container.x;
            auto dy = start_offset.y - container.y;
            if (dx < 0 || dy < 0) return { .container = box };

            auto x = container.x + dx;
            auto y = container.y + dy;

            auto padding_left = style.padding.left.as_cell() + style.border_left.border_width();
            auto padding_right = style.padding.right.as_cell() + style.border_right.border_width();

            auto required_space = static_cast<int>(next_word_length);
            if (padding_right > 0 || style.margin.right.as_cell() > 0) required_space = 0;

            auto len = static_cast<int>(core::utf8::calculate_size(text));

            x += padding_left;

            if (container.max_x() - (x + padding_right) < 0) {
                box.width = std::max(0, container.max_x() - (x + padding_right));
                return { .container = box };
            }

            if (x + len + padding_right + required_space < container.max_x()) {
                box.height = 1;
                auto end_x = device.write_text(text, x, y, core::PixelStyle::from_style(style)).second;
                auto w = end_x - x;
                box.width = w + padding_right + required_space;
                return { .container = box, .text_rendered = text.size() };
            }

            box.x = container.x;
            box.width = std::min(
                width + padding_right + padding_left + required_space,
                container.width
            );

            auto cell_style = core::PixelStyle::from_style(style);

            auto start = std::size_t{};
            auto start_y = y;
            auto extra_space = padding_right;
            do {
                if (y + 1 >= container.max_y()) {
                    extra_space = required_space + padding_right;
                }

                if (std::isspace(text[start])) {
                    if (x + 1 >= container.max_x()) {
                        ++y;
                        x = container.min_x();
                        if (y >= container.max_y()) break;
                    }
                    auto render_whitespace = (
                        style.whitespace == style::Whitespace::Pre ||
                        style.whitespace == style::Whitespace::PreWrap ||
                        x != container.min_x()
                    );
                    if (render_whitespace) {
                        auto x_inc = 1;
                        if (text[start] == '\n') {
                            x = container.min_x();
                            x_inc = 0;
                            ++y;
                        }
                        device.put_pixel(" ", x, y, cell_style);
                        x += x_inc;
                    }
                    start += 1;
                }
                auto pos = detail::find_word(text, start);
                auto txt = text.substr(start, pos - start);
                auto sz = static_cast<int>(core::utf8::calculate_size(txt));

                if (x - dx + sz + extra_space > container.max_x()) {
                    if (x != container.min_x()) {
                        ++y;
                        x = container.min_x();
                        if (y >= container.max_y()) break;
                    }
                }

                bool rendered = false;
                if (style.overflow_wrap == style::OverflowWrap::BreakWord) {
                    if (x - dx + sz + extra_space > container.max_x()) {
                        for (auto i = 0ul; i < text.size(); ++x) {
                            auto l = core::utf8::get_length(text[i]);
                            if (x + static_cast<int>(l) + extra_space >= container.max_x()) {
                                ++y;
                                x = container.min_x();

                                if (y >= container.max_y()) break;
                                if (y + 1 >= container.max_y()) {
                                    extra_space = required_space + padding_right;
                                }
                            }

                            assert(i + l <= text.size());

                            device.write_text(text.substr(i, l), x, y, cell_style);

                            i += l;
                        }
                        rendered = true;
                    }
                }

                if (!rendered) {
                    x = device.write_text(txt, x, y, cell_style).second;
                }

                start = pos;
            } while (start < text.size());

            box.height = y - start_y + 1;
            start_offset = {
                .x = x,
                .y = y
            };

            return { .container = box, .text_rendered = text.size() };
        };
    };

} // namespace termml::text

#endif // AMT_TERMML_TEXT_HPP
