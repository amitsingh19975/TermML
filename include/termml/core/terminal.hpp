#ifndef AMT_THERMML_CORE_TERMINAL_HPP
#define AMT_THERMML_CORE_TERMINAL_HPP

#include "device.hpp"
#include "commands.hpp"
#include "utf8.hpp"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <vector>

namespace termml::core {

    struct Terminal {
        struct Cell {
            PixelStyle style{};

            char buff[4]{};
            std::uint8_t len{};

            bool is_dirty{true};

            constexpr auto set_text(std::string_view text) noexcept -> void {
                if (this->text() == text) return;
                is_dirty = true;

                if (text.empty()) {
                    len = 0;
                    return;
                }

                auto size = utf8::get_length(text[0]);
                assert(size <= text.size());
                std::copy_n(text.begin(), size, buff);
                len = size;
            }

            constexpr auto text() const noexcept -> std::string_view {
                return { buff, len };
            }
        };

        Terminal(int rows, int cols)
            : m_rows(static_cast<unsigned>(std::max(0, rows)))
            , m_cols(static_cast<unsigned>(std::max(0, cols)))
            , m_data(m_cols * m_rows, Cell())
        {}

        Terminal(Terminal const&) = delete;
        Terminal(Terminal &&) noexcept = default;
        Terminal& operator=(Terminal const&) = delete;
        Terminal& operator=(Terminal &&) noexcept = default;
        ~Terminal() = default;

        constexpr auto rows() const noexcept -> int { return static_cast<int>(m_rows); }
        constexpr auto cols() const noexcept -> int { return static_cast<int>(m_cols); }
        constexpr auto operator()(unsigned r, unsigned c) const noexcept -> Cell const& {
            assert(r < m_rows);
            assert(c < m_cols);
            return m_data[r * m_cols + c];
        }
        constexpr auto operator()(unsigned r, unsigned c) noexcept -> Cell& {
            assert(r < m_rows);
            assert(c < m_cols);
            return m_data[r * m_cols + c];
        }

        constexpr auto put_pixel(
            std::string_view pixel,
            int x, int y,
            PixelStyle const& style = {}
        ) -> void {
            if (y >= rows() || y < 0) return;
            if (x >= cols() || x < 0) return;
            auto& cell = this->operator()(static_cast<unsigned>(y), static_cast<unsigned>(x));
            if (cell.style.z_index > style.z_index) return;
            if (cell.style != style) cell.is_dirty = true;
            cell.style = style;
            cell.set_text(pixel);
        }

        constexpr auto clear() -> void {
            std::fill(m_data.begin(), m_data.end(), Cell{});
        }

        auto flush(Command& cmd) -> void {
            auto previous_style = PixelStyle{};
            auto previous_pos = std::make_pair(0u, 0u);
            for (auto r = 0u; r < m_rows; ++r) {
                for (auto c = 0u; c < m_cols; ++c) {
                    auto& cell = this->operator()(r, c);
                    if (!cell.is_dirty) continue;
                    auto [pr, pc] = previous_pos;
                    if (!((pr == r) && (pc + 1 == c))) {
                        cmd.move(c, r);
                    }

                    if (!previous_style.is_same_style(cell.style)) {
                        cmd.reset();
                        write_style(cmd, cell.style);
                    }

                    cmd.write(cell.text());
                    previous_style = cell.style;
                    cell.is_dirty = false;
                    previous_pos = { r, c };
                }
            }
        }
    private:
        static auto write_style(Command& cmd, PixelStyle const& style) -> void {
            { // text color
                if (style.fg_color.is_rgb()) {
                    auto rgb = style.fg_color.as_rgb();
                    cmd.write_rgb(rgb.r, rgb.g, rgb.b, true);
                } else if (style.fg_color.is_4bit()) {
                    cmd.write_basic_color(style.fg_color.as_bit(), true);
                } else if (style.fg_color.is_8bit()) {
                    cmd.write_8bit_color(style.fg_color.as_bit(), true);
                }
            }

            { // text color
                if (style.bg_color.is_rgb()) {
                    auto rgb = style.bg_color.as_rgb();
                    cmd.write_rgb(rgb.r, rgb.g, rgb.b, false);
                } else if (style.bg_color.is_4bit()) {
                    cmd.write_basic_color(style.bg_color.as_bit(), false);
                } else if (style.bg_color.is_8bit()) {
                    cmd.write_8bit_color(style.bg_color.as_bit(), false);
                }
            }

            if (style.bold) {
                cmd.bold();
            }

            if (style.dim) {
                cmd.dim();
            }

            if (style.italic) {
                cmd.italic();
            }

            if (style.underline) {
                cmd.underline();
            }
        }
    private:
        unsigned m_rows{};
        unsigned m_cols{};
        std::vector<Cell> m_data;
    };

    static_assert(detail::IsScreen<Terminal>);
} // namespace termml::core

#endif // AMT_THERMML_CORE_TERMINAL_HPP
