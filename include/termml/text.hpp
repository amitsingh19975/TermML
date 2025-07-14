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
        style::Whitespace whitespace{style::Whitespace::Normal};
        core::BoundingBox container{};
        core::Point offset{};

        auto render() const -> core::BoundingBox {
            
        };
    };

} // namespace termml::text

#endif // AMT_TERMML_TEXT_HPP
