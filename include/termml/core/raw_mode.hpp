#ifndef AMT_TERMML_CORE_RAW_MODE_HPP
#define AMT_TERMML_CORE_RAW_MODE_HPP

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string_view>
#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#    define WIN32_IS_MEAN_WAS_LOCALLY_DEFINED
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#    define NOMINMAX_WAS_LOCALLY_DEFINED
#  endif
#
#  include <windows.h>
#
#  ifdef WIN32_IS_MEAN_WAS_LOCALLY_DEFINED
#    undef WIN32_IS_MEAN_WAS_LOCALLY_DEFINED
#    undef WIN32_LEAN_AND_MEAN
#  endif
#  ifdef NOMINMAX_WAS_LOCALLY_DEFINED
#    undef NOMINMAX_WAS_LOCALLY_DEFINED
#    undef NOMINMAX
#  endif

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#error "ANSI escape sequence is not supported."
#endif
#else
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/ttycom.h>
#endif

namespace termml::core {

    namespace detail {
#ifdef _WIN32
        std::string wideToUtf8(std::wstring_view wstr) {
            if (wstr.empty()) return {};

            int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0,
                wstr.data(), static_cast<int>(wstr.size()),
                nullptr, 0, nullptr, nullptr);
            std::string result(static_cast<std::size_t>(sizeNeeded), 0);
            WideCharToMultiByte(CP_UTF8, 0,
                wstr.data(), static_cast<int>(wstr.size()),
                &result[0], sizeNeeded, nullptr, nullptr);
            return result;
        }

        constexpr auto from_virtual_key(WORD vk, char* buffer, bool is_ctrl) noexcept -> std::ptrdiff_t {
            auto it = buffer;
            switch (vk) {
                case VK_UP:     it = std::format_to(buffer, "\x1b[{}", is_ctrl ? "5A" : "A"); break;
                case VK_DOWN:   it = std::format_to(buffer, "\x1b[{}", is_ctrl ? "5B" : "B"); break;
                case VK_RIGHT:  it = std::format_to(buffer, "\x1b[{}", is_ctrl ? "5C" : "C"); break;
                case VK_LEFT:   it = std::format_to(buffer, "\x1b[{}", is_ctrl ? "5D" : "D"); break;
                case VK_HOME:   it = std::format_to(buffer, "\x1b[H"); break;
                case VK_END:    it = std::format_to(buffer, "\x1b[F"); break;
                case VK_INSERT: it = std::format_to(buffer, "\x1b[2~"); break;
                case VK_DELETE: it = std::format_to(buffer, "\x1b[3~"); break;
                case VK_PRIOR:  it = std::format_to(buffer, "\x1b[5~"); break;
                case VK_NEXT:   it = std::format_to(buffer, "\x1b[6~"); break;
                case VK_F1: it = std::format_to(buffer, "\x1bOP"); break;
                case VK_F2: it = std::format_to(buffer, "\x1bOQ"); break;
                case VK_F3: it = std::format_to(buffer, "\x1bOR"); break;
                case VK_F4: it = std::format_to(buffer, "\x1bOS"); break;
                case VK_F5: it = std::format_to(buffer, "\x1b[15~"); break;
                case VK_F6: it = std::format_to(buffer, "\x1b[17~"); break;
                case VK_F7: it = std::format_to(buffer, "\x1b[18~"); break;
                case VK_F8: it = std::format_to(buffer, "\x1b[19~"); break;
                case VK_F9: it = std::format_to(buffer, "\x1b[20~"); break;
                case VK_F10: it = std::format_to(buffer, "\x1b[21~"); break;
                case VK_F11: it = std::format_to(buffer, "\x1b[22~"); break;
                case VK_F12: it = std::format_to(buffer, "\x1b[23~"); break;
                default:
                    return 0; // Unknown or unhandled key, skip
            }
            return (it - buffer);
        }
#endif
    }

    struct RawModeGuard {
        RawModeGuard() {
#ifdef _WIN32
            hStdin = GetStdHandle(STD_INPUT_HANDLE);
            GetConsoleMode(hStdin, &origMode);
            DWORD mode = origMode;
            mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_MOUSE_INPUT | ENABLE_PROCESSED_INPUT);
            SetConsoleMode(hStdin, mode);
#else
            tcgetattr(STDIN_FILENO, &origTermios);
            termios raw = origTermios;
            raw.c_lflag &= ~static_cast<tcflag_t>(ECHO | ICANON | IEXTEN | ISIG);
            raw.c_iflag &= ~static_cast<tcflag_t>(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
            raw.c_cflag |= (CS8);
            raw.c_oflag &= ~static_cast<tcflag_t>(OPOST);
            raw.c_cc[VMIN] = 0;
            raw.c_cc[VTIME] = 0;
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
            static constexpr auto mouse_tracking = std::string_view("\x1b[?1000h");
            static constexpr auto mouse_tracking_extended = std::string_view("\x1b[?1006h");
            static constexpr auto any_motion = std::string_view("\x1b[?1003h");
            write(STDIN_FILENO, mouse_tracking.data(), mouse_tracking.size());
            write(STDIN_FILENO, mouse_tracking_extended.data(), mouse_tracking_extended.size());
            write(STDIN_FILENO, any_motion.data(), any_motion.size());
#endif
            if (!enable_ansi_escape_seq()) {
                throw std::runtime_error("ANSI escape sequence is not supported.");
            }
        }

        ~RawModeGuard() {
#ifdef _WIN32
            SetConsoleMode(hStdin, origMode);
#else
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &origTermios);
#endif
        }

        auto read(char* buffer, std::size_t len) const noexcept -> std::ptrdiff_t {
#ifdef _WIN32
            HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
            INPUT_RECORD record;
            DWORD count;

            static constexpr DWORD wbuf_size = 256;
            wchar_t wbuf[wbuf_size];
            DWORD wread = 0;
            DWORD wait_result = WaitForSingleObject(hIn, 0);

            if (wait_result != WAIT_OBJECT_0) return 0;

            if (!ReadConsoleInput(hIn, &record, 1, &count)) {
                return -1;
            }
            if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown) {
                WCHAR wc = record.Event.KeyEvent.uChar.UnicodeChar;
                if (wc != 0) {
                    char utf8buf[5]{};
                    int n = WideCharToMultiByte(CP_UTF8, 0, &wc, 1, utf8buf, sizeof(utf8buf), nullptr, nullptr);
                    if (n > 0 && static_cast<std::size_t>(n) <= len) {
                        std::memcpy(buffer, utf8buf, n);
                        return static_cast<std::ptrdiff_t>(n);
                    } else {
                        return -1;
                    }
                } else {
                    WORD vk = record.Event.KeyEvent.wVirtualKeyCode;
                    DWORD mods = record.Event.KeyEvent.dwControlKeyState;
                    auto ctrl = static_cast<bool>(mods & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED));
                    auto size = detail::from_virtual_key(vk, buffer, ctrl);
                    return std::min(static_cast<std::ptrdiff_t>(len), size);
                }
            } else if (record.EventType == MOUSE_EVENT) {
                const MOUSE_EVENT_RECORD& me = record.Event.MouseEvent;
                int x = me.dwMousePosition.X + 1;
                int y = me.dwMousePosition.Y + 1;
                int button = 0;
                char action = 'M';

                if (me.dwEventFlags == MOUSE_WHEELED) {
                    button = (GET_WHEEL_DELTA_WPARAM(me.dwButtonState) > 0) ? 64 : 65;
                }
                else if (me.dwButtonState == 0) {
                    button = 3;
                    action = 'm';
                }
                else {
                    if (me.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) button = 0;
                    else if (me.dwButtonState & RIGHTMOST_BUTTON_PRESSED) button = 2;
                    else if (me.dwButtonState & FROM_LEFT_2ND_BUTTON_PRESSED) button = 1;
                    else button = 3;
                }

                auto it = std::format_to(buffer, "\x1b[<{};{};{}{}", button, x, y, action);
                return it - buffer;
            }
            return 0;
#else
            return ::read(STDIN_FILENO, buffer, len);
#endif
        }
    private:
#ifdef _WIN32
        static auto enable_ansi_escape_seq() noexcept -> bool {
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            if (hOut == INVALID_HANDLE_VALUE) return false;

            DWORD dwMode = 0;
            if (!GetConsoleMode(hOut, &dwMode)) return false;

            // Try enabling ENABLE_VIRTUAL_TERMINAL_PROCESSING
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            if (!SetConsoleMode(hOut, dwMode)) return false;

            return true;
        }
#else
        static auto enable_ansi_escape_seq() noexcept -> bool { return true; }
#endif

    private:
#ifdef _WIN32
        HANDLE hStdin;
        DWORD origMode;
#else
        termios origTermios;
#endif
    };

    static inline auto is_displayed(int fd) noexcept -> bool {
        #ifndef _WIN32
            return isatty(fd);
        #else
            DWORD Mode;
            return (GetConsoleMode(reinterpret_cast<HANDLE>(_get_osfhandle(fd)), &Mode) != 0);
        #endif
    }

    static inline auto get_fd_from_handle(FILE* handle) noexcept -> int {
        #ifndef _WIN32
            return fileno(handle);
        #else
            return _fileno(handle);
        #endif
    }

    static inline auto is_displayed(FILE* handle) noexcept -> bool {
        return is_displayed(get_fd_from_handle(handle));
    }

    static inline auto get_columns(int fd) noexcept -> std::size_t {
        #ifndef _WIN32
            const char* cols_str = std::getenv("COLUMNS");
            if (cols_str) {
                int cols = std::atoi(cols_str);
                if (cols > 0) return static_cast<std::size_t>(cols);
            } else {
                winsize win;
                if (ioctl(fd, TIOCGWINSZ, &win) >= 0) {
                    return win.ws_col;
                }
            }
            return 0zu;
        #else
            HANDLE handle = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
            unsigned cols = 0;
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            if (GetConsoleScreenBufferInfo(handle, &csbi)) cols = csbi.dwSize.X;
            return static_cast<std::size_t>(cols);
        #endif
    }

    static inline auto get_columns(FILE* handle) noexcept -> std::size_t {
        return get_columns(get_fd_from_handle(handle));
    }
} // namespace termml::core

#endif // AMT_TERMML_CORE_RAW_MODE_HPP

