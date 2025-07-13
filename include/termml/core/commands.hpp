#ifndef AMT_TERMML_CORE_COMMANDS_HPP
#define AMT_TERMML_CORE_COMMANDS_HPP

#include "raw_mode.hpp"
#include <cstdint>
#include <cstdio>
#include <print>

namespace termml::core {
    struct Command {
        constexpr Command(FILE* handle = stdout) noexcept
            : m_handle(handle)
            , m_is_displayed(is_displayed(handle))
        {}

        constexpr Command(Command const&) noexcept = default;
        constexpr Command(Command &&) noexcept = default;
        constexpr Command& operator=(Command const&) noexcept = default;
        constexpr Command& operator=(Command &&) noexcept = default;
        constexpr ~Command() noexcept = default;

        template <typename... Args>
        auto write(std::format_string<Args...> fmt, Args&&... args) -> Command& {
            std::print(m_handle, fmt, std::forward<Args>(args)...);
            return *this;
        }

        auto write(std::string_view str) -> Command& {
            std::print(m_handle, "{}", str);
            return *this;
        }

        auto reset() -> Command& {
            if (!m_is_displayed) return *this;
            write("\x1b[0m");
            return *this;
        }

        auto clear_screen() -> Command& {
            if (!m_is_displayed) return *this;
            write("\x1b[2J");
            return *this;
        }

        auto move(unsigned x, unsigned y) -> Command& {
            if (!m_is_displayed) return *this;
            write("\x1b[{};{}H", y, x);
            return *this;
        }

        auto move_to_start() -> Command& {
            if (!m_is_displayed) return *this;
            write("\x1b[H");
            return *this;
        }

        auto save_cursor() -> Command& {
            if (!m_is_displayed) return *this;
            write("\x1b[s");
            return *this;
        }

        auto restore_cursor() -> Command& {
            if (!m_is_displayed) return *this;
            write("\x1b[u");
            return *this;
        }

        auto clear_line() -> Command& {
            if (!m_is_displayed) return *this;
            write("\x1b[2K");
            return *this;
        }

        auto hide_cursor(bool flag = true) -> Command& {
            if (!m_is_displayed) return *this;
            write("\x1b[?25{}", flag ? 'l' : 'h');
            return *this;
        }

        auto write_rgb(std::uint8_t r, std::uint8_t g, std::uint8_t b, bool fg) -> Command& {
            if (!m_is_displayed) return *this;
            write("\x1b[{};2;{};{};{}m", fg ? 38 : 48, r, g, b);
            return *this;
        }

        auto write_8bit_color(std::uint8_t c, bool fg) -> Command& {
            if (!m_is_displayed) return *this;
            write("\x1b[{};5;{}m", fg ? 38 : 48, c);
            return *this;
        }

        auto write_basic_color(std::uint8_t c, bool fg) -> Command& {
            if (!m_is_displayed) return *this;
            unsigned code;
            if (c > 7) {
                code = 90 + (c % 8) + (fg ? 0 : 10);
            } else {
                code = 30 + (c % 8) + (fg ? 0 : 10);
            }

            if (c == 16) {
                code = fg ? 39 : 49;
            }
            write("\x1b[{}m", code);
            return *this;
        }

        auto bold() -> Command& {
            if (!m_is_displayed) return *this;
            write("\x1b[1m");
            return *this;
        }

        auto dim() -> Command& {
            if (!m_is_displayed) return *this;
            write("\x1b[2m");
            return *this;
        }

        auto italic() -> Command& {
            if (!m_is_displayed) return *this;
            write("\x1b[3m");
            return *this;
        }

        auto underline() -> Command& {
            if (!m_is_displayed) return *this;
            write("\x1b[4m");
            return *this;
        }

        auto underline(std::uint8_t c) -> Command& {
            if (!m_is_displayed) return *this;
            write("\x1b[4;5;{}m", c);
            return *this;
        }

        auto underline(std::uint8_t r, std::uint8_t g, std::uint8_t b) -> Command& {
            if (!m_is_displayed) return *this;
            write("\x1b[4;2;{};{};{}m", r, g, b);
            return *this;
        }

        auto flush() -> Command& {
            std::fflush(m_handle);
            return *this;
        }

        auto request_window_size() -> Command& {
            if (!m_is_displayed) return *this;
            save_cursor();
            write("\x1b[999;999H\x1b[6n");
            restore_cursor();
            return *this;
        }
    private:
        FILE* m_handle;
        bool m_is_displayed{false};
    };
} // namespace termml::core

#endif // AMT_TERMML_CORE_COMMANDS_HPP
