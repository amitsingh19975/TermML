#ifndef AMT_THERMML_CORE_DEVICE_HPP
#define AMT_THERMML_CORE_DEVICE_HPP

#include "../style.hpp"
#include "point.hpp"
#include "commands.hpp"
#include <format>
#include <limits>
#include <string_view>

namespace termml::core {

    struct PixelStyle {
        style::Color fg_color{style::Color::Default};
        style::Color bg_color{style::Color::Default};
        bool bold{false};
        bool dim{false};
        bool italic{false};
        bool underline{false};
        int z_index{};

        static constexpr auto from_style(style::Style const& style) noexcept -> PixelStyle {
            return {
                .fg_color = style.fg_color,
                .bg_color = style.bg_color,
                // TODO: add font style properties to the style
                .z_index = style.z_index
            };
        }

        constexpr auto operator==(PixelStyle const&) const noexcept -> bool = default;

        constexpr auto is_same_style(PixelStyle const& other) const noexcept -> bool {
            return (
                fg_color == other.fg_color &&
                bg_color == other.bg_color &&
                bold == other.bold &&
                dim == other.dim &&
                italic == other.italic &&
                underline == other.underline
            );
        }
        
        auto dump() const -> std::string {
            return std::format("PixelStyle(fg: {}, bg: {}, bold: {}, dim: {}, italic: {}, underline: {}, z_index: {})", fg_color, bg_color, bold, dim, italic, underline, z_index);
        }
    };

    namespace detail {
        template <typename T>
        concept IsScreen = requires (T& t, T const& v, Command& cmd) {
            { t.put_pixel(std::string_view{}, 0, 0, PixelStyle{}) };
            { t.clear() };
            { t.flush(cmd) };
            { v.rows() } -> std::same_as<int>;
            { v.cols() } -> std::same_as<int>;
        };
    } // namespace detail

    template <detail::IsScreen S>
    struct Device {
        Device(S&& screen)
            : m_screen(std::move(screen))
        {}

        constexpr Device(Device const&) = delete;
        constexpr Device(Device &&) noexcept = default;
        constexpr Device& operator=(Device const&) = delete;
        constexpr Device& operator=(Device &&) noexcept = default;
        constexpr ~Device() = default;

        constexpr auto put_pixel(
            std::string_view pixel,
            int x, int y,
            PixelStyle const& p = {}
        ) -> Device& {
            m_screen.put_pixel(pixel, x, y, p);
            return *this;
        }

        constexpr auto put_pixel(
            std::string_view pixel,
            Point coord,
            PixelStyle const& p = {}
        ) -> Device& {
            m_screen.put_pixel(pixel, coord.x, coord.y, p);
            return *this;
        }

        constexpr auto clear() -> Device& {
            m_screen.clear();
            return *this;
        }

        constexpr auto flush(Command& cmd) -> void {
            m_screen.flush(cmd);
        }

        constexpr auto rows() const noexcept -> int {
            return m_screen.rows();
        }

        constexpr auto cols() const noexcept -> int {
            return m_screen.cols();
        }

    private:
        S m_screen;
    };


    struct NullScreen {
        constexpr auto put_pixel(
            [[maybe_unused]] std::string_view pixel,
            [[maybe_unused]] int x,
            [[maybe_unused]] int y,
            [[maybe_unused]] PixelStyle const& p = {}
        ) noexcept -> void {}

        constexpr auto clear() noexcept -> void {}

        constexpr auto flush([[maybe_unused]] Command& cmd) noexcept -> void {}

        constexpr auto rows() const noexcept -> int {
            return std::numeric_limits<int>::max();
        }

        constexpr auto cols() const noexcept -> int {
            return std::numeric_limits<int>::max();
        }
    };

    using null_device_t = Device<NullScreen>;

} // namespace termml::core

#endif // AMT_THERMML_CORE_DEVICE_HPP
