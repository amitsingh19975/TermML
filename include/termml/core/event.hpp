#ifndef AMT_TERMML_CORE_EVENT_HPP
#define AMT_TERMML_CORE_EVENT_HPP

#include "basic.hpp"
#include "raw_mode.hpp"
#include "utf8.hpp"
#include <algorithm>
#include <cctype>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <variant>
#include <cassert>

namespace termml {

    struct KeyboardMod {
        static constexpr std::uint8_t Shift = 1;
        static constexpr std::uint8_t Alt   = 2;
        static constexpr std::uint8_t Ctrl  = 4;
    };

    enum class KeyboardKey {
        None,
        Escape,
        UP,
        Down,
        Right,
        Left,
        Home,
        End,
        Insert,
        Delete,
        Prior,
        Next,
        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,
    };

    struct KeyboardEvent {
        std::uint8_t mod{};
        KeyboardKey key{};
        char buff[4]{};
        std::uint8_t len{};

        constexpr auto is_shift_pressed() const noexcept -> bool {
            return mod & KeyboardMod::Shift;
        }

        constexpr auto is_ctrl_pressed() const noexcept -> bool {
            return mod & KeyboardMod::Ctrl;
        }

        constexpr auto is_alt_pressed() const noexcept -> bool {
            return mod & KeyboardMod::Alt;
        }

        constexpr auto is_ascii() const noexcept -> bool {
            return (!empty()) && (buff[0] <= 0x7F);
        }

        constexpr auto empty() const noexcept -> bool {
            return len == 0;
        }

        constexpr auto size() const noexcept -> std::size_t {
            return len;
        }

        constexpr auto str() const noexcept -> std::string_view {
            return { buff, size() };
        }
    };


    enum class MouseButton: std::uint8_t {
        None = 0,
        LeftDown = 1, // M
        LeftUp = 2, // m
        MiddleDown = 3,
        MiddleUp = 4,
        RightDown = 5,
        RightUp = 6
    };

    enum class ScrollDirection: std::uint8_t {
        None,
        Up,
        Down
    };

    struct MouseEvent {
        MouseButton button{};
        ScrollDirection scroll_dir{};
        unsigned x;
        unsigned y;
    };

    struct TerminateEvent {};

    struct WindowSize {
        unsigned rows{};
        unsigned cols{};
    };

    struct Event {
        constexpr Event() noexcept = default;
        constexpr Event(Event const&) noexcept = default;
        constexpr Event(Event &&) noexcept = default;
        constexpr Event& operator=(Event const&) noexcept = default;
        constexpr Event& operator=(Event &&) noexcept = default;
        constexpr ~Event() noexcept = default;

        constexpr Event(KeyboardEvent const& e) noexcept
            : m_event(e)
        {}

        constexpr Event(MouseEvent const& e) noexcept
            : m_event(e)
        {}

        constexpr Event(TerminateEvent const& e) noexcept
            : m_event(e)
        {}

        constexpr Event(WindowSize const& e) noexcept
            : m_event(e)
        {}

        template <typename T>
        constexpr auto is() const noexcept -> bool {
            return std::holds_alternative<T>(m_event);
        }

        constexpr auto visit(auto&& fn) const noexcept {
            return std::visit(std::forward<decltype(fn)>(fn), m_event);
        }

        constexpr auto empty() const noexcept -> bool {
            return std::holds_alternative<std::monostate>(m_event);
        }

        template <typename T>
        constexpr auto as() const noexcept -> T const& {
            assert(std::holds_alternative<T>(m_event));
            return std::get<T>(m_event);
        }

        template <typename T>
        constexpr auto as() noexcept -> T& {
            assert(std::holds_alternative<T>(m_event));
            return std::get<T>(m_event);
        }

        static constexpr auto parse(core::RawModeGuard const& r) noexcept -> Event {
            char buffer[32]{};
            auto size = r.read(buffer, 32);
            if (size < 0) return TerminateEvent();
            if (size == 0) return Event();
            auto code = std::string_view(buffer, static_cast<std::size_t>(size));

            auto tmp = core::trim_escape_seq(code);
            if (tmp.empty()) return Event();

            if (tmp.size() == code.size()) {
                auto e = KeyboardEvent {};
                if (code[0] == 0x1B /*Alt esc*/) {
                    code = code.substr(1);
                    if (code.empty()) {
                        e.key = KeyboardKey::Escape;
                        return Event(e);
                    }
                    e.mod |= KeyboardMod::Alt;
                }

                if (std::iscntrl(code[0])) {
                    switch (code[0]) {
                        case '\n': case '\r': case ' ': break;
                        default: {
                            e.mod |= KeyboardMod::Ctrl;
                            buffer[0] = buffer[0] + 'a' - 1;
                        }
                    } 
                }

                auto len = core::utf8::get_length(code[0]);
                assert(len <= code.size());
                e.len = len;

                if (len == 1) {
                    e.buff[0] = code[0];
                    if (std::isupper(code[0])) {
                        e.mod |= KeyboardMod::Shift;
                        e.buff[0] = static_cast<char>(std::tolower(code[0]));
                    }
                } else {
                    // TODO: test for shift key. We need utf8 to utf32 and back converter.
                    std::copy_n(code.begin(), len, e.buff);
                }

                return Event(e);
            } else {
                // special characters
                if (tmp[0] == '<') {
                    // Mouse Events
                    tmp = tmp.substr(1);
                    if (tmp.empty()) return Event();
                    int button = 0, x = 0, y = 0;
                    char type = 'M';

                    if (std::sscanf(tmp.data(), "%d;%d;%d%c", &button, &x, &y, &type) != 4) {
                        return Event();
                    }

                    auto e = MouseEvent {
                        .x = static_cast<unsigned>(x),
                        .y = static_cast<unsigned>(y)
                    };

                    switch (button & 0x11) {
                        case 0: {
                            e.button = type == 'M' ? MouseButton::LeftDown : MouseButton::LeftUp;
                        } break;
                        case 1: {
                            e.button = type == 'M' ? MouseButton::MiddleDown : MouseButton::MiddleUp;
                        } break;
                        case 2: {
                            e.button = type == 'M' ? MouseButton::RightDown : MouseButton::RightUp;
                        } break;
                    }

                    auto scroll = (button & 0b11111100);
                    if (scroll == 64) {
                        e.scroll_dir = ScrollDirection::Up;
                    } else if (scroll == 65) {
                        e.scroll_dir = ScrollDirection::Down;
                    }

                    return Event(e);
                } else if (tmp.ends_with("R")) {
                    int rows = -1, cols = -1;
                    if (std::sscanf(tmp.data(), "%d;%dR", &rows, &cols) != 2) {
                        return Event();
                    }

                    if (rows < 0 || cols < 0) return {};

                    return Event(
                        WindowSize(
                            static_cast<unsigned>(rows),
                            static_cast<unsigned>(cols)
                        )
                    );
                } else if (tmp.starts_with("5")) {
                    tmp = tmp.substr(1);
                    if (tmp.empty()) return {};
                    auto k = KeyboardEvent{ .mod = KeyboardMod::Ctrl };
                    switch (tmp[0]) {
                        case 'A': k.key = KeyboardKey::UP; break;
                        case 'B': k.key = KeyboardKey::Down; break;
                        case 'C': k.key = KeyboardKey::Right; break;
                        case 'D': k.key = KeyboardKey::Left; break;
                    }
                    return Event(k);
                } else if ((tmp[0] >= 'A' && tmp[0] <= 'D')) {
                    auto k = KeyboardEvent{};
                    switch (tmp[0]) {
                        case 'A': k.key = KeyboardKey::UP; break;
                        case 'B': k.key = KeyboardKey::Down; break;
                        case 'C': k.key = KeyboardKey::Right; break;
                        case 'D': k.key = KeyboardKey::Left; break;
                    }
                    return Event(k);
                }
                return Event();
            }
        }

    private:
        std::variant<std::monostate, KeyboardEvent, MouseEvent, TerminateEvent, WindowSize> m_event{};
    };
} // namespace termml

#include <format>

template <>
struct std::formatter<termml::KeyboardEvent> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(termml::KeyboardEvent const& e, auto& ctx) const {
        using termml::KeyboardMod, termml::KeyboardKey;
        auto out = ctx.out();
        std::format_to(out, "KeyboardEvent(mod=[");
        if (e.mod & KeyboardMod::Shift) {
            std::format_to(out, "Shift, ");
        }
        if (e.mod & KeyboardMod::Alt) {
            std::format_to(out, "Alt, ");
        }
        if (e.mod & KeyboardMod::Ctrl) {
            std::format_to(out, "Ctrl, ");
        }
        std::format_to(out, "], text='{}', key=", e.str());
        switch (e.key) {
            case KeyboardKey::None: std::format_to(out, "None");
            case KeyboardKey::Escape: std::format_to(out, "Escape"); break;
            case KeyboardKey::UP: std::format_to(out, "UP"); break;
            case KeyboardKey::Down: std::format_to(out, "Down"); break;
            case KeyboardKey::Right: std::format_to(out, "Right"); break;
            case KeyboardKey::Left: std::format_to(out, "Left"); break;
            case KeyboardKey::Home: std::format_to(out, "Home"); break;
            case KeyboardKey::End: std::format_to(out, "End"); break;
            case KeyboardKey::Insert: std::format_to(out, "Insert"); break;
            case KeyboardKey::Delete: std::format_to(out, "Delete"); break;
            case KeyboardKey::Prior: std::format_to(out, "Prior"); break;
            case KeyboardKey::Next: std::format_to(out, "Next"); break;
            case KeyboardKey::F1: std::format_to(out, "F1"); break;
            case KeyboardKey::F2: std::format_to(out, "F2"); break;
            case KeyboardKey::F3: std::format_to(out, "F3"); break;
            case KeyboardKey::F4: std::format_to(out, "F4"); break;
            case KeyboardKey::F5: std::format_to(out, "F5"); break;
            case KeyboardKey::F6: std::format_to(out, "F6"); break;
            case KeyboardKey::F7: std::format_to(out, "F7"); break;
            case KeyboardKey::F8: std::format_to(out, "F8"); break;
            case KeyboardKey::F9: std::format_to(out, "F9"); break;
            case KeyboardKey::F10: std::format_to(out, "F10"); break;
            case KeyboardKey::F11: std::format_to(out, "F11"); break;
            case KeyboardKey::F12: std::format_to(out, "F12"); break;
        }
        std::format_to(out, ")");
        return out;
    }
};

template <>
struct std::formatter<termml::MouseEvent> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(termml::MouseEvent const& e, auto& ctx) const {
        using termml::MouseButton, termml::ScrollDirection;
        auto out = ctx.out();
        std::format_to(out, "MouseEvent(button=");
        switch (e.button) {
            case MouseButton::LeftDown: std::format_to(out, "LeftDown"); break;
            case MouseButton::LeftUp: std::format_to(out, "LeftUp"); break;
            case MouseButton::MiddleDown: std::format_to(out, "MiddleDown"); break;
            case MouseButton::MiddleUp: std::format_to(out, "MiddleUp"); break;
            case MouseButton::RightDown: std::format_to(out, "RightDown"); break;
            case MouseButton::RightUp: std::format_to(out, "RightUp"); break;
            default: std::format_to(out, "None"); break;
        }

        std::format_to(out, ", scroll_dir=");
        switch (e.scroll_dir) {
            case ScrollDirection::Up: std::format_to(out, "Up"); break;
            case ScrollDirection::Down: std::format_to(out, "Down"); break;
            default: std::format_to(out, "None"); break;
        }
        std::format_to(out, ", x={}, y={})", e.x, e.y);
        return out;
    }
};

template <>
struct std::formatter<termml::TerminateEvent> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(termml::TerminateEvent const&, auto& ctx) const {
        return std::format_to(ctx.out(), "TerminateEvent");
    }
};

template <>
struct std::formatter<termml::WindowSize> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(termml::WindowSize const& e, auto& ctx) const {
        return std::format_to(ctx.out(), "WindowSize(rows: {}, cols: {})", e.rows, e.cols);
    }
};

template <>
struct std::formatter<termml::Event> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(termml::Event const& et, auto& ctx) const {
        auto out = ctx.out();
        et.visit([&out]<typename T>(T const& e){
            if constexpr (std::same_as<std::monostate, T>) {
                std::format_to(out, "Event(<None>)");
            } else {
                std::format_to(out, "Event({})", e);
            }
        });
        return out;
    }
};
#endif // AMT_TERMML_CORE_EVENT_HPP
