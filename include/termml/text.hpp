#ifndef AMT_TERMML_TEXT_HPP
#define AMT_TERMML_TEXT_HPP

#include "style.hpp"
#include "core/commands.hpp"
#include "core/bounding_box.hpp"
#include "core/point.hpp"

namespace termml::text {

    struct TextRenderResult {
        core::BoundingBox container;
        std::size_t text_rendered{};
        bool overflowed_x{false};
        bool overflowed_y{false};
    };

    struct TextRenderer {
        std::string_view text;
        core::Command& command = core::Command::null();
        core::BoundingBox viewport{core::BoundingBox::inf()};
        core::Point start_offset{};
        core::Point scroll_offset{};

        // Message content width; ignore padding and margin
        constexpr auto measure_width() const noexcept -> int {
            int width{};
            auto start = std::size_t{};
            auto it = std::min(text.find('\n'), text.size());
            while (it < text.size()) {
                auto w = static_cast<int>(it - start);
                width = std::max(width, w);
                start = it + 1;
                it = std::min(text.find('\n', start), text.size());
            }

            auto w = static_cast<int>(it - start);
            width = std::max(width, w);

            return width;
        }

        constexpr auto measure_height(style::Style const& style) const noexcept -> int {
            (void)style; 
            return 0;
        }

        auto render(style::Style const& style) const -> core::BoundingBox {
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

            (void) style;
            return {};
        };
    };

} // namespace termml::text

#endif // AMT_TERMML_TEXT_HPP
