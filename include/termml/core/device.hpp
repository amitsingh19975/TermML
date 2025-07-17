#ifndef AMT_THERMML_CORE_DEVICE_HPP
#define AMT_THERMML_CORE_DEVICE_HPP

#include "../style.hpp"
#include "point.hpp"
#include "commands.hpp"
#include "bounding_box.hpp"
#include "utf8.hpp"
#include <cassert>
#include <cstdint>
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
            { t.put_pixel(std::string_view{}, 0, 0, PixelStyle{}) } -> std::same_as<bool>;
            { t.clear() };
            { t.flush(cmd, 0u, 0u) };
            { v.rows() } -> std::same_as<int>;
            { v.cols() } -> std::same_as<int>;
        };
    } // namespace detail

    struct NullScreen {
        int ncols{std::numeric_limits<int>::max()};
        int nrows{std::numeric_limits<int>::max()};

        constexpr auto put_pixel(
            [[maybe_unused]] std::string_view pixel,
            [[maybe_unused]] int x,
            [[maybe_unused]] int y,
            [[maybe_unused]] PixelStyle const& p
        ) noexcept -> bool {
            return x < cols() && y < rows();
        }

        constexpr auto clear() noexcept -> void {}

        constexpr auto flush([[maybe_unused]] Command& cmd, unsigned, unsigned) noexcept -> void {}

        constexpr auto rows() const noexcept -> int {
            return nrows;
        }

        constexpr auto cols() const noexcept -> int {
            return ncols;
        }
    };

    static_assert(detail::IsScreen<NullScreen>);

    template <detail::IsScreen S>
    struct Device {
        enum class PutPixelResult: std::uint8_t {
            OutOfBound,
            Rendered,
            Clipped
        };

        Device(S* screen)
            : m_screen(screen)
        {}

        constexpr Device(Device const&) = default;
        constexpr Device(Device &&) noexcept = default;
        constexpr Device& operator=(Device const&) = default;
        constexpr Device& operator=(Device &&) noexcept = default;
        constexpr ~Device() = default;

        constexpr auto put_pixel(
            std::string_view pixel,
            int x, int y,
            PixelStyle const& p = {}
        ) noexcept -> PutPixelResult {
            if constexpr (!std::same_as<S, NullScreen>) {
                if (!m_viewport.in(x, y)) return PutPixelResult::Clipped;
            }
            return m_screen->put_pixel(pixel, x, y, p) ? PutPixelResult::Rendered : PutPixelResult::OutOfBound;
        }

        constexpr auto put_pixel(
            std::string_view pixel,
            Point coord,
            PixelStyle const& p = {}
        ) noexcept -> PutPixelResult {
            return put_pixel(pixel, coord.x, coord.y, p);
        }

        constexpr auto clear() -> Device& {
            m_screen->clear();
            return *this;
        }

        constexpr auto flush(Command& cmd, unsigned dx = 0, unsigned dy = 0) -> void {
            m_screen->flush(cmd, dx, dy);
        }

        constexpr auto rows() const noexcept -> int {
            return m_screen->rows();
        }

        constexpr auto cols() const noexcept -> int {
            return m_screen->cols();
        }

        constexpr auto write_text(
            std::string_view text,
            int x,
            int y,
            PixelStyle const& p = {}
        ) noexcept -> std::pair<std::size_t /*text rendered*/, int /*pixels consumed*/> {
            auto i = std::size_t{};
            if (y >= m_viewport.max_y()) return { 0, x };
            for (; i < text.size(); ++x) {
                auto len = ::termml::core::utf8::get_length(text[i]);
                assert(i + len <= text.size());
                auto txt = text.substr(i, len);
                if (put_pixel(txt, x, y, p) == PutPixelResult::OutOfBound) break;
                if (x >= m_viewport.max_x()) break;
                i += len;
            }
            return { i, x };
        }

        constexpr auto clip(BoundingBox viewport = BoundingBox::inf()) noexcept -> void {
            m_viewport = viewport;
        }

        constexpr auto viewport() const noexcept -> BoundingBox { return m_viewport; }

    private:
        S* m_screen;
        BoundingBox m_viewport{BoundingBox::inf()};
    };


    using null_device_t = Device<NullScreen>;

    inline static constexpr auto null_device() noexcept -> null_device_t& {
        static auto screen = NullScreen{};
        static auto instance = Device<NullScreen>{&screen};
        return instance;
    }

    template <detail::IsScreen S>
    struct ViewportClipGuard {
        constexpr ViewportClipGuard(Device<S>& d, BoundingBox viewport) noexcept
            : m_d(d)
            , m_old_viewport(m_d.viewport())
        {
            m_d.clip(viewport);
        }

        constexpr ViewportClipGuard(ViewportClipGuard const&) = delete;
        constexpr ViewportClipGuard(ViewportClipGuard &&) = delete;
        constexpr ViewportClipGuard& operator=(ViewportClipGuard const&) = delete;
        constexpr ViewportClipGuard& operator=(ViewportClipGuard &&) = delete;

        constexpr ~ViewportClipGuard() noexcept {
            m_d.clip(m_old_viewport);
        }
    private:
        Device<S>& m_d;
        BoundingBox m_old_viewport;
    };

} // namespace termml::core

#endif // AMT_THERMML_CORE_DEVICE_HPP
