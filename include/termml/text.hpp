#ifndef AMT_TERMML_TEXT_HPP
#define AMT_TERMML_TEXT_HPP

#include "style.hpp"
#include "core/bounding_box.hpp"
#include "core/point.hpp"
#include "core/device.hpp"
#include "core/utf8.hpp"

namespace termml::text {

    struct TextRenderResult {
        core::BoundingBox container;
        std::size_t text_rendered{};
        bool overflowed_x{false};
        bool overflowed_y{false};
    };

    struct TextRenderer {
        std::string_view text;
        // Scroll container
        core::BoundingBox container{core::BoundingBox::inf()};
        // Viewport that will be visible
        core::BoundingBox viewport{core::BoundingBox::inf()};
        // position within the scroll container
        core::Point start_offset{};

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
            return render(core::null_device(), style).height;
        }

        template <core::detail::IsScreen T>
        auto render(
            core::Device<T>& device,
            style::Style const& style
        ) const -> core::BoundingBox {
            core::ViewportClipGuard clip(device, viewport);

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

            auto width = style.content_width();
            auto box = viewport;
            box.width = 0;
            box.height = 0;
            if (width == 0) return box;

            auto dx = container.x - start_offset.x;
            // auto y = container.y - start_offset.y;

            auto len = static_cast<int>(core::utf8::calculate_size(text));

            std::println("HERE: {} + {} < {}", dx, len, width);

            if (dx + len < width) {
                box.height = style.display == style::Display::Block ? 1 : 0;
                box.width = device.write_text(text, start_offset.x, start_offset.y, core::PixelStyle::from_style(style)).second;
                return box;
            }

            return box;
        };
    };

} // namespace termml::text

#endif // AMT_TERMML_TEXT_HPP
