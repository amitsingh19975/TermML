#ifndef AMT_TERMML_CSS_STYLE_HPP
#define AMT_TERMML_CSS_STYLE_HPP

#include "../core/color_utils.hpp"
#include "../core/string_utils.hpp"
#include "utils.hpp"
#include <array>
#include <charconv>
#include <cstdlib>
#include <span>
#include <unordered_map>

namespace termml::css {
    struct CSSPropertyKey {
        static constexpr std::string_view color = "color";
        static constexpr std::string_view background_color = "background-color";

        static constexpr std::string_view padding           = "padding";
        static constexpr std::string_view padding_left      = "padding-left";
        static constexpr std::string_view padding_right     = "padding-right";
        static constexpr std::string_view padding_top       = "padding-top";
        static constexpr std::string_view padding_bottom    = "padding-bottom";

        static constexpr std::string_view margin        = "margin";
        static constexpr std::string_view margin_left   = "margin-left";
        static constexpr std::string_view margin_right  = "margin-right";
        static constexpr std::string_view margin_top    = "margin-top";
        static constexpr std::string_view margin_bottom = "margin-bottom";

        static constexpr std::string_view width         = "width";
        static constexpr std::string_view min_width     = "min-width";
        static constexpr std::string_view max_width     = "max-width";
        static constexpr std::string_view height        = "height";
        static constexpr std::string_view min_height    = "min-height";
        static constexpr std::string_view max_height    = "max-height";

        static constexpr std::string_view border           = "border";
        static constexpr std::string_view border_left      = "border-left";
        static constexpr std::string_view border_right     = "border-right";
        static constexpr std::string_view border_top       = "border-top";
        static constexpr std::string_view border_bottom    = "border-bottom";

        static constexpr std::string_view border_type                = "border-type";
        static constexpr std::string_view border_type_top_left       = "border-type-top-left";
        static constexpr std::string_view border_type_top_right      = "border-type-top-right";
        static constexpr std::string_view border_type_bottom_left    = "border-type-bottom-left";
        static constexpr std::string_view border_type_bottom_right   = "border-type-bottom-right";

        static constexpr std::string_view inset = "inset";
        static constexpr std::string_view top = "top";
        static constexpr std::string_view left = "left";
        static constexpr std::string_view right = "right";
        static constexpr std::string_view bottom = "bottom";

        static constexpr std::string_view z_index = "z_index";
        static constexpr std::string_view display = "display";
        static constexpr std::string_view whitespace = "white-space";

        static constexpr std::string_view overflow = "overflow";
        static constexpr std::string_view overflow_x = "overflow_x";
        static constexpr std::string_view overflow_y = "overflow_y";

        static constexpr auto is_inheritable(std::string_view key) noexcept -> bool {
            if (key == color) return true;
            if (key == background_color) return true;
            if (key == whitespace) return true;
            return false;
        }

        static constexpr std::array inherited_properties = {
            color, background_color, whitespace
        };
    }; 

    struct RGBColor {
        std::uint8_t r;
        std::uint8_t g;
        std::uint8_t b;

        constexpr auto operator==(RGBColor const&) const noexcept -> bool = default;
    };

    struct Color {
        enum ColorKind: std::uint8_t {
            RGB,
            BIT4,
            BIT8,
            TRANSPARENT
        };

        struct bit4_tag {};

        static const Color Black;
        static const Color Red;
        static const Color Green;
        static const Color Yellow;
        static const Color Blue;
        static const Color Magenta;
        static const Color Cyan;
        static const Color White;
        static const Color BrightBlack;
        static const Color BrightRed;
        static const Color BrightGreen;
        static const Color BrightYellow;
        static const Color BrightBlue;
        static const Color BrightMagenta;
        static const Color BrightCyan;
        static const Color BrightWhite;
        static const Color Default;
        static const Color Transparent;

        constexpr Color() noexcept
            : m_kind(TRANSPARENT)
        {}

        constexpr Color(std::uint8_t r, std::uint8_t g, std::uint8_t b) noexcept
            : m_data{ .rgb = {r, g, b} }
            , m_kind(ColorKind::RGB)
        {}

        constexpr Color(RGBColor rgb) noexcept
            : m_data{ .rgb = rgb }
            , m_kind(ColorKind::RGB)
        {}

        constexpr Color(std::uint8_t bit8) noexcept
            : m_data { .bit = bit8 }
            , m_kind(Color::BIT8)
        {}

        constexpr Color(std::uint8_t bit4, bit4_tag) noexcept
            : m_data { .bit = bit4 }
            , m_kind(Color::BIT4)
        {}

        constexpr auto as_rgb() const noexcept -> RGBColor {
            return m_data.rgb;
        };

        constexpr auto as_bit() const noexcept -> std::uint8_t {
            return m_data.bit;
        };

        constexpr auto is_rgb() const noexcept -> bool { return m_kind == RGB; }
        constexpr auto is_8bit() const noexcept -> bool { return m_kind == BIT8; }
        constexpr auto is_4bit() const noexcept -> bool { return m_kind == BIT4; }
        constexpr auto is_transparent() const noexcept -> bool { return m_kind == TRANSPARENT; }

        static constexpr auto parse(std::string_view c, Color def = Color::Default) noexcept -> Color {
            c = core::utils::trim(c);
            if (c.empty()) return def;
            // hex color -> rgb
            // named color -> bit4
            // bit8(xx) -> 8-bit color

            if (c.starts_with("#")) {
                c = c.substr(1);
                auto len = c.size();
                // #rrggbb
                auto i = 0ul;
                auto k = 0ul;
                std::array<std::uint8_t, 3> tmp{};
                if (len % 2 == 1) {
                    std::from_chars(c.data(), c.data() + 1, tmp[k++], 16);
                    i += 1;
                }

                for (; i < len && k < 3; i += 2) {
                    std::from_chars(c.data() + i, c.data() + i + 2, tmp[k++], 16);
                }

                return Color(tmp[0], tmp[1], tmp[2]);
            } else if (c.starts_with("rgb")) {
                c = c.substr(3);
                std::array<std::uint8_t, 3> tmp{};
                parse_args(c, std::span(tmp.data(), tmp.size()));
                return Color(tmp[0], tmp[1], tmp[2]);
            } else if (c.starts_with("bit")) {
                c = c.substr(3);
                std::array<std::uint8_t, 1> tmp{};
                parse_args(c, std::span(tmp.data(), tmp.size()));
                return Color(tmp[0]);
            } else if (c.starts_with("hsl")) {
                c = c.substr(3);
                std::array<float, 3> tmp{};
                parse_args(c, std::span(tmp.data(), tmp.size()));
                auto [r, g, b] = core::hsl_to_rgb(tmp[0], tmp[1], tmp[2]);
                return Color(r, g, b);
            } else {
                if (c == "transparent") return Color::Transparent;
                if (c == "default") return Color::Default;
                if (c == "black") return Color::Black;
                if (c == "red") return Color::Red;
                if (c == "green") return Color::Green;
                if (c == "yellow") return Color::Yellow;
                if (c == "blue") return Color::Blue;
                if (c == "magenta") return Color::Magenta;
                if (c == "cyan") return Color::Cyan;
                if (c == "white") return Color::White;
                if (c == "light-black") return Color::BrightBlack;
                if (c == "light-red") return Color::BrightRed;
                if (c == "light-green") return Color::BrightGreen;
                if (c == "light-yellow") return Color::BrightYellow;
                if (c == "light-blue") return Color::BrightBlue;
                if (c == "light-magenta") return Color::BrightMagenta;
                if (c == "light-cyan") return Color::BrightCyan;
                if (c == "light-white") return Color::BrightWhite;
            }

            return def;
        }

        constexpr auto operator==(Color const& other) const noexcept -> bool {
            if (m_kind != other.m_kind) return false;
            if (m_kind == RGB) return as_rgb() == other.as_rgb();
            return as_bit() == other.as_bit();
        }
    private:
        template <typename T>
            requires std::is_arithmetic_v<T>
        static constexpr auto parse_args(std::string_view c, std::span<T> out, int base = 10) noexcept -> void {
            auto k = 0ul;
            auto i = 0ul;
            char buffer[64]{};
            auto len = std::min<std::size_t>(c.size(), 63);
            std::copy_n(c.begin(), len, buffer);
            buffer[len] = 0;

            c = std::string_view(buffer, len);

            // TODO: To better float parsing.
            while (k < out.size() && i < c.size()) {
                if (c[i] == ')') return;
                for (; i < c.size(); ++i) {
                    auto t = c[i];
                    if (t == ')') return;
                    if (std::isdigit(t) || t == '.') break;
                }

                auto start = i;

                for (; i < c.size(); ++i) {
                    if (!std::isdigit(c[i]) && c[i] != '.') break;
                }

                if constexpr (std::is_integral_v<T>) {
                    std::from_chars(c.data() + start, c.data() + i, out[k++], base);
                } else {
                    char* end{};
                    out[k++] = std::strtof(c.data() + start, &end);
                }
            }
        }
    private:
        union {
            RGBColor rgb;
            std::uint8_t bit;
        } m_data{};
        ColorKind m_kind{};
    };

    inline constexpr auto Color::Black              = Color{ 0, Color::bit4_tag{} };
    inline constexpr auto Color::Red                = Color{ 1, Color::bit4_tag{} };
    inline constexpr auto Color::Green              = Color{ 2, Color::bit4_tag{} };
    inline constexpr auto Color::Yellow             = Color{ 3, Color::bit4_tag{} };
    inline constexpr auto Color::Blue               = Color{ 4, Color::bit4_tag{} };
    inline constexpr auto Color::Magenta            = Color{ 5, Color::bit4_tag{} };
    inline constexpr auto Color::Cyan               = Color{ 6, Color::bit4_tag{} };
    inline constexpr auto Color::White              = Color{ 7, Color::bit4_tag{} };
    inline constexpr auto Color::BrightBlack        = Color{ 8, Color::bit4_tag{} };
    inline constexpr auto Color::BrightRed          = Color{ 9, Color::bit4_tag{} };
    inline constexpr auto Color::BrightGreen        = Color{10, Color::bit4_tag{} };
    inline constexpr auto Color::BrightYellow       = Color{11, Color::bit4_tag{} };
    inline constexpr auto Color::BrightBlue         = Color{12, Color::bit4_tag{} };
    inline constexpr auto Color::BrightMagenta      = Color{13, Color::bit4_tag{} };
    inline constexpr auto Color::BrightCyan         = Color{14, Color::bit4_tag{} };
    inline constexpr auto Color::BrightWhite        = Color{15, Color::bit4_tag{} };
    inline constexpr auto Color::Default            = Color{16, Color::bit4_tag{} };
    inline constexpr auto Color::Transparent        = Color{};


    enum class Unit {
        Auto,
        Percentage,
        Cell
    };

    struct Number {
        union {
            float f; // %
            int i; // cell
        };
        Unit unit{};

        static constexpr auto parse(std::string_view s, Number def = Number::fit()) noexcept -> Number {
            s = core::utils::trim(s);
            if (s.empty() || s == "fit") return def;

            if (s.ends_with('%')) {
                char* end{};
                float tmp = std::strtof(s.data(), &end);
                return { .f = tmp, .unit = Unit::Percentage };
            } else {
                auto i = int{};
                auto j = 0ul;
                bool is_neg = false;
                if (s.starts_with('-')) {
                    is_neg = true;
                    s = core::utils::ltrim(s.substr(1));
                }
                if (s.empty()) return def;

                for (; j < s.size(); ++j) {
                    if (!std::isdigit(s[j])) break;
                }
                std::from_chars(s.data(), s.data() + j, i);
                auto u = s.substr(j);
                if (u == "px" || u == "c" || u == "cell") {
                    return { .i = is_neg ? -i : i, .unit = Unit::Cell };
                } else {
                    return def;
                }
            }
        }

        static constexpr auto fit() noexcept -> Number {
            return { .i = 0, .unit = Unit::Auto };
        };

        static constexpr auto min() noexcept -> Number {
            return { .i = 0, .unit = Unit::Cell };
        };

        static constexpr auto max() noexcept -> Number {
            return { .i = std::numeric_limits<int>::max(), .unit = Unit::Cell };
        };

        constexpr auto resolve_percentage(int val) const noexcept -> Number {
            if (unit != Unit::Percentage) return *this;
            return {
                .i = static_cast<int>(
                    static_cast<float>(val) * f / 100.f
                ),
                .unit = Unit::Cell
            };
        }

        constexpr auto resolve_all(int val) const noexcept -> Number {
            if (unit == Unit::Auto) {
                return {
                    .i = val,
                    .unit = Unit::Cell
                };
            }
            return resolve_percentage(val);
        }

        constexpr auto is_absolute() const noexcept -> bool {
            return unit == Unit::Cell;
        }

        constexpr auto is_precentage() const noexcept -> bool {
            return unit == Unit::Percentage;
        }

        constexpr auto is_fit() const noexcept -> bool {
            return unit == Unit::Auto;
        }

        static constexpr auto from_cell(int cell) noexcept -> Number {
            return { .i = cell, .unit = Unit::Cell };
        }

        constexpr auto as_cell() const noexcept -> int {
            return is_absolute() ? i : 0;
        }
    };

    struct QuadProperty {
        Number top{Number::min()};
        Number right{Number::min()};
        Number bottom{Number::min()};
        Number left{Number::min()};

        constexpr auto resolve(int val) noexcept -> QuadProperty {
            return {
                .top = top.resolve_all(val),
                .right = right.resolve_all(val),
                .bottom = bottom.resolve_all(val),
                .left = left.resolve_all(val)
            };
        }

        constexpr auto vertical() const noexcept -> int { return top.as_cell() + bottom.as_cell(); }
        constexpr auto horizontal() const noexcept -> int { return left.as_cell() + right.as_cell(); }
    };

    // top, right, bottom, left
    inline static constexpr auto parse_quad_values(std::string_view s, QuadProperty def = QuadProperty{}) noexcept -> QuadProperty {
        s = core::utils::trim(s);
        if (s.empty()) return def;

        auto k = 0ul;
        std::array<Number, 4> tmp;
        auto i = 0ul;

        for (; i < s.size(); ++i) {
            if (!std::isspace(s[i])) break;
        }

        for (; i < s.size() && k < 4;) {
            auto start = i;
            for (; i < s.size(); ++i) {
                if (std::isspace(s[i])) break;
            }

            tmp[k++] = Number::parse(s.substr(start, i - start), Number::min());

            for (; i < s.size(); ++i) {
                if (!std::isspace(s[i])) break;
            }
        }

        switch (k) {
            case 1: return QuadProperty(tmp[0], tmp[0], tmp[0], tmp[0]);
            case 2: return QuadProperty(tmp[0], tmp[1], tmp[0], tmp[1]);
            case 3: return QuadProperty(tmp[0], tmp[1], tmp[2], Number::min());
            case 4: return QuadProperty(tmp[0], tmp[1], tmp[2], tmp[3]);
        }

        std::unreachable();
    }

    enum class BorderStyle {
        None,
        Solid,
        Dotted
    };

    enum class BorderType {
        Sharp,
        Rounded
    };


    inline static constexpr auto parse_border_style(std::string_view s, BorderStyle def = BorderStyle::None) noexcept -> BorderStyle {
        if (s == "solid") return BorderStyle::Solid;
        if (s == "dotted") return BorderStyle::Dotted;
        return def;
    }

    struct Border {
        Number width{ Number::min() };
        BorderStyle style{};
        Color color{Color::Default};

        static constexpr auto parse(std::string_view s) noexcept -> Border {
            s = core::utils::trim(s);
            if (s.empty()) return {};

            auto width = Number::fit();
            // border: [thin/thick] solid black
            auto it = s.find(' ');
            auto prefix = s.substr(0, it);
            if (prefix == "thin") {
                width = Number::from_cell(1);
                s = s.substr(it + 1);
            } else if (prefix == "thick") {
                width = Number::from_cell(2);
                s = s.substr(it + 1);
            }

            // enable this if we figure out how to control border width using cells
            // if (std::isdigit(s[0])) {
            //     // border: 1 solid black
            //     auto i = std::size_t{};
            //     for (; i < s.size(); ++i) {
            //         if (std::isspace(s[i])) break;
            //     }
            //
            //     width = Number::parse(s.substr(0, i));
            //
            //     for (; i < s.size(); ++i) {
            //         if (!std::isspace(s[i])) break;
            //     }
            //     s = s.substr(i);
            // }

            // border: solid
            // border: solid red
            auto i = std::size_t{};
            for (; i < s.size(); ++i) {
                if (std::isspace(s[i])) break;
            }

            auto style = parse_border_style(s.substr(0, i));

            for (; i < s.size(); ++i) {
                if (!std::isspace(s[i])) break;
            }

            auto color = Color::parse(s.substr(std::min(i, s.size())));
            return { .width = width, .style = style, .color = color };
        };

        constexpr auto border_width() const noexcept -> int {
            return std::min(width.as_cell(), 1);
        }

        constexpr auto is_thick() const noexcept -> bool { return width.as_cell() == 2; }
        constexpr auto is_thin() const noexcept -> bool { return width.as_cell() == 1; }

        constexpr auto char_set(BorderType type) const noexcept -> utils::BoxCharSet {
            auto horizontal = utils::char_set::box::rounded.horizonal;
            auto vertical = utils::char_set::box::rounded.vertical;
            auto tl = utils::char_set::box::dotted.top_left;
            auto tr = utils::char_set::box::dotted.top_right;
            auto br = utils::char_set::box::dotted.bottom_right;
            auto bl = utils::char_set::box::dotted.bottom_left;

            if (type == BorderType::Rounded) {
                tl = utils::char_set::box::rounded.top_left;
                tr = utils::char_set::box::rounded.top_right;
                br = utils::char_set::box::rounded.bottom_right;
                bl = utils::char_set::box::rounded.bottom_left;
            }

            if (style == BorderStyle::Dotted) {
                horizontal = utils::char_set::box::dotted.horizonal;
                vertical = utils::char_set::box::dotted.vertical;
            }

            if (width.as_cell() == 2) {
                if (style == BorderStyle::Dotted) {
                    horizontal = utils::char_set::box::dotted_bold.horizonal;
                    vertical = utils::char_set::box::dotted_bold.vertical;
                } else {
                    horizontal = utils::char_set::box::rounded_bold.horizonal;
                    vertical = utils::char_set::box::rounded_bold.vertical;
                }
                if (type == BorderType::Rounded) {
                    tl = utils::char_set::box::rounded_bold.top_left;
                    tr = utils::char_set::box::rounded_bold.top_right;
                    br = utils::char_set::box::rounded_bold.bottom_right;
                    bl = utils::char_set::box::rounded_bold.bottom_left;
                } else {
                    tl = utils::char_set::box::dotted_bold.top_left;
                    tr = utils::char_set::box::dotted_bold.top_right;
                    br = utils::char_set::box::dotted_bold.bottom_right;
                    bl = utils::char_set::box::dotted_bold.bottom_left;
                }
            }

            return {
                .vertical = vertical,
                .horizonal = horizontal,
                .top_left = tl,
                .top_right = tr,
                .bottom_right = br,
                .bottom_left = bl,
                .left_connector = "",
                .top_connector = "",
                .right_connector = "",
                .bottom_connector = ""
            };
        }
    };

    inline static constexpr auto parse_border_type(std::string_view s, BorderType def = BorderType::Sharp) noexcept -> std::tuple<BorderType, BorderType, BorderType, BorderType> {
        s = core::utils::trim(s);
        if (s.empty()) return { def, def, def, def };

        auto k = 0ul;
        std::array<BorderType, 4> tmp;
        auto i = 0ul;

        for (; i < s.size(); ++i) {
            if (!std::isspace(s[i])) break;
        }

        for (; i < s.size() && k < 4;) {
            auto start = i;
            for (; i < s.size(); ++i) {
                if (std::isspace(s[i])) break;
            }

            auto text = s.substr(start, i - start);
            if (text == "rounded") {
                tmp[k++] = BorderType::Rounded;
            } else {
                tmp[k++] = BorderType::Sharp;
            }

            for (; i < s.size(); ++i) {
                if (!std::isspace(s[i])) break;
            }
        }

        switch (k) {
            case 1: return std::make_tuple(tmp[0], tmp[0], tmp[0], tmp[0]);
            case 2: return std::make_tuple(tmp[0], tmp[1], tmp[0], tmp[1]);
            case 3: return std::make_tuple(tmp[0], tmp[1], tmp[2], BorderType::Sharp);
            case 4: return std::make_tuple(tmp[0], tmp[1], tmp[2], tmp[3]);
        }

        std::unreachable();
    }

    enum class Overflow {
        Visible,
        Auto,
        Clip,
        Scroll
    };

    inline static constexpr auto parse_overflow(std::string_view s, Overflow def = Overflow::Clip) noexcept -> Overflow {
        s = core::utils::trim(s);
        if (s.empty()) return def;

        if (s == "clip") return Overflow::Clip;
        if (s == "auto") return Overflow::Auto;
        if (s == "visible") return Overflow::Visible;
        if (s == "scroll") return Overflow::Scroll;

        return def;
    }

    enum class Whitespace {
        Normal, // All the whitespace characters are collapsed to ' '
        NoWrap,
        Pre, // Everything is preserved
        PreWrap, // Everything is preversed with text wrapping
        PreLine, // Newline is preserved but other whitespace charactors are collapsed.
    };

    enum class OverflowWrap {
        Normal, // Lines may only break at normal word break points (such as a space between two words).
        BreakWord
    };

    struct TextStyle {
        bool bold{};
        bool italic{};
        bool dim{};
        bool underline{};
        bool strike{};
    };

    enum class Display {
        Block,
        InlineBlock,
        Inline,
        Flex,
        Grid,
    };

    enum class ItemType {
        None,
        Flex,
        Grid
    };

    struct Style {
        Number min_width{ Number::min() };
        Number max_width{ Number::max() };

        Number min_height{ Number::min() };
        Number max_height{ Number::max() };

        Number width { Number::fit() };
        Number height { Number::fit() };

        Display display{Display::Block};
        ItemType item_type{ItemType::None};

        Border border_top{};
        Border border_right{};
        Border border_bottom{};
        Border border_left{};

        std::tuple<
            BorderType /*tl*/,
            BorderType /*tr*/,
            BorderType /*br*/,
            BorderType /*bl*/
        > border_type{ BorderType::Sharp, BorderType::Sharp, BorderType::Sharp, BorderType::Sharp };

        QuadProperty padding{};
        QuadProperty margin{};
        QuadProperty inset{};

        int z_index{};

        Overflow overflow_x{Overflow::Visible};
        Overflow overflow_y{Overflow::Visible};

        Color fg_color{Color::Default};
        Color bg_color{Color::Default};

        Whitespace whitespace{Whitespace::Normal};
        OverflowWrap overflow_wrap{OverflowWrap::Normal};

        TextStyle text_style{};

        constexpr auto parse_proprties(
            std::string_view tag,
            std::unordered_map<std::string_view, std::string_view> const& props,
            Style const* parent = nullptr
        ) noexcept -> void {

            {
                auto d = core::utils::trim(get_property(props, CSSPropertyKey::display));
                if (d == "block") display = Display::Block;
                else if (d == "inline") display = Display::Inline;
                else if (d == "inline-block") display = Display::InlineBlock;
                else if (d == "flex") display = Display::Flex;
                else {
                    if (tag == "text" || tag == "span" || tag == "em") display = Display::Inline;
                    else if (tag == "b" || tag == "strong" || tag == "i") display = Display::Inline;
                }

                if (parent) {
                    switch (parent->display) {
                    case Display::Flex: item_type = ItemType::Flex; break;
                    case Display::Grid: item_type = ItemType::Grid; break;
                    default: item_type = ItemType::None;
                    }
                }
            }

            fg_color = Color::parse(
                get_property(props, CSSPropertyKey::color),
                parent ? parent->fg_color : Color::Default
            );

            bg_color = Color::parse(
                get_property(props, CSSPropertyKey::background_color),
                parent ? parent->bg_color : Color::Default
            );

            // padding
            {
                auto tp = get_property(props, CSSPropertyKey::padding); 
                auto tp_top = get_property(props, CSSPropertyKey::padding_top); 
                auto tp_right = get_property(props, CSSPropertyKey::padding_right); 
                auto tp_bottom = get_property(props, CSSPropertyKey::padding_bottom); 
                auto tp_left = get_property(props, CSSPropertyKey::padding_left); 
                if (!tp.empty()) {
                    padding = parse_quad_values(tp);
                }
                if (!tp_top.empty()) {
                    padding.top = parse_quad_values(tp_top).top;
                }
                if (!tp_right.empty()) {
                    padding.right = parse_quad_values(tp_right).right;
                }
                if (!tp_bottom.empty()) {
                    padding.bottom = parse_quad_values(tp_bottom).bottom;
                }
                if (!tp_left.empty()) {
                    padding.left = parse_quad_values(tp_left).left;
                }
            }

            // margin
            {
                auto tm = get_property(props, CSSPropertyKey::margin); 
                auto tm_top = get_property(props, CSSPropertyKey::margin_top); 
                auto tm_right = get_property(props, CSSPropertyKey::margin_right); 
                auto tm_bottom = get_property(props, CSSPropertyKey::margin_bottom); 
                auto tm_left = get_property(props, CSSPropertyKey::margin_left); 
                if (!tm.empty()) {
                    margin = parse_quad_values(tm);
                }
                if (!tm_top.empty()) {
                    margin.top = parse_quad_values(tm_top).top;
                }
                if (!tm_right.empty()) {
                    margin.right = parse_quad_values(tm_right).right;
                }
                if (!tm_bottom.empty()) {
                    margin.bottom = parse_quad_values(tm_bottom).bottom;
                }
                if (!tm_left.empty()) {
                    margin.left = parse_quad_values(tm_left).left;
                }
            }

            // border
            {
                auto tb = get_property(props, CSSPropertyKey::border); 
                auto tb_top = get_property(props, CSSPropertyKey::border_top); 
                auto tb_right = get_property(props, CSSPropertyKey::border_right); 
                auto tb_bottom = get_property(props, CSSPropertyKey::border_bottom); 
                auto tb_left = get_property(props, CSSPropertyKey::border_left); 
                if (!tb.empty()) {
                    // TODO: add support for multiple border parsing separated by ','
                    auto b = Border::parse(tb);
                    border_top = b;
                    border_right = b;
                    border_bottom = b;
                    border_left = b;
                }
                if (!tb_top.empty()) {
                    border_top = Border::parse(tb_top);
                }
                if (!tb_right.empty()) {
                    border_right = Border::parse(tb_right);
                }
                if (!tb_bottom.empty()) {
                    border_bottom = Border::parse(tb_bottom);
                }
                if (!tb_left.empty()) {
                    border_left = Border::parse(tb_left);
                }

                auto bt = get_property(props, CSSPropertyKey::border_type);
                auto bt_tl = get_property(props, CSSPropertyKey::border_type_top_left);
                auto bt_tr = get_property(props, CSSPropertyKey::border_type_top_right);
                auto bt_br = get_property(props, CSSPropertyKey::border_type_bottom_right);
                auto bt_bl = get_property(props, CSSPropertyKey::border_type_bottom_left);

                if (!bt.empty()) {
                    auto b = parse_border_type(bt);
                    border_type = b;
                }
                if (!bt_tl.empty()) {
                    auto b = parse_border_type(bt_tl);
                    std::get<0>(border_type) = std::get<0>(b);
                }

                if (!bt_tr.empty()) {
                    auto b = parse_border_type(bt_tr);
                    std::get<1>(border_type) = std::get<1>(b);
                }

                if (!bt_br.empty()) {
                    auto b = parse_border_type(bt_br);
                    std::get<2>(border_type) = std::get<2>(b);
                }

                if (!bt_bl.empty()) {
                    auto b = parse_border_type(bt_bl);
                    std::get<3>(border_type) = std::get<3>(b);
                }
            }

            // insert
            {
                auto ti = get_property(props, CSSPropertyKey::inset); 
                auto ti_top = get_property(props, CSSPropertyKey::top); 
                auto ti_right = get_property(props, CSSPropertyKey::right); 
                auto ti_bottom = get_property(props, CSSPropertyKey::bottom); 
                auto ti_left = get_property(props, CSSPropertyKey::left); 
                if (!ti.empty()) {
                    inset = parse_quad_values(ti);
                }
                if (!ti_top.empty()) {
                    inset.top = parse_quad_values(ti_top).top;
                }
                if (!ti_right.empty()) {
                    inset.right = parse_quad_values(ti_right).right;
                }
                if (!ti_bottom.empty()) {
                    inset.bottom = parse_quad_values(ti_bottom).bottom;
                }
                if (!ti_left.empty()) {
                    inset.left = parse_quad_values(ti_left).left;
                }
            }

            auto tw = get_property(props, CSSPropertyKey::width);
            if (!tw.empty()) {
                width = Number::parse(tw);
            } else {
                if (display == Display::Block) {
                    width = { .f = 100, .unit = Unit::Percentage };
                }
            }

            auto th = get_property(props, CSSPropertyKey::height);
            if (!th.empty()) {
                height = Number::parse(th);
            }

            min_width = Number::parse(
                get_property(props, CSSPropertyKey::min_width),
                Number::min()
            );

            min_height = Number::parse(
                get_property(props, CSSPropertyKey::min_height),
                Number::min()
            );

            max_width = Number::parse(
                get_property(props, CSSPropertyKey::max_width),
                Number::max()
            );

            max_height = Number::parse(
                get_property(props, CSSPropertyKey::max_height),
                Number::max()
            );

            {
                auto tz = Number::parse(get_property(props, CSSPropertyKey::z_index));
                if (tz.is_absolute()) {
                    z_index = tz.i;
                }
            }

            {
                auto to = core::utils::trim(get_property(props, CSSPropertyKey::overflow));
                auto to_x = get_property(props, CSSPropertyKey::overflow_x);
                auto to_y = get_property(props, CSSPropertyKey::overflow_y);

                if (!to.empty()) {
                    auto space_pos = to.find(' ');
                    overflow_y = parse_overflow(to.substr(0, space_pos));
                    overflow_x = parse_overflow(to.substr(space_pos + 1));
                }

                if (!to_x.empty()) {
                    overflow_x = parse_overflow(to_x, overflow_x);
                }

                if (!to_y.empty()) {
                    overflow_y = parse_overflow(to_y, overflow_y);
                }
            }
            // white-space
            {
                auto ws = core::utils::trim(get_property(props, CSSPropertyKey::whitespace));

                if (ws == "normal") whitespace = Whitespace::Normal;
                else if (ws == "nowrap") whitespace = Whitespace::NoWrap;
                else if (ws == "pre") whitespace = Whitespace::Pre;
                else if (ws == "pre-line") whitespace = Whitespace::PreLine;
                else if (ws == "pre-wrap") whitespace = Whitespace::PreWrap;
            }

            // text style
            {
                text_style.bold = (tag == "b");
                text_style.italic = (tag == "i");
            }

            if (display == Display::Inline) {
                width = Number::fit();
                height = Number::fit();
                min_width = Number::min();
                min_height = Number::min();
                max_width = Number::max();
                max_height = Number::max();
                margin.top = Number::min();
                margin.bottom = Number::min();
            }
        }

        constexpr auto content_width() const noexcept -> int {
            auto w = width.as_cell();
            auto b = border_left.border_width() + border_right.border_width();
            auto p = padding.left.as_cell() + padding.right.as_cell();
            return std::max(
                0,
                w - (b + p)
            );
        }

        static constexpr auto has_inline_flow(Display d) noexcept -> bool {
            switch (d) {
            case Display::InlineBlock: return true;
            case Display::Inline: return true;
            case Display::Flex: return false; // TODO: add axis properties
            default: return false;
            }
        }
        constexpr auto has_inline_flow() const noexcept -> bool { return has_inline_flow(display); }

        static constexpr auto is_inline_context(Display d) noexcept -> bool {
            switch (d) {
            case Display::InlineBlock: return true;
            case Display::Inline: return true;
            default: return false;
            }
        }
        constexpr auto is_inline_context() const noexcept -> bool { return is_inline_context(display); }

        static constexpr auto has_start_whitespace(QuadProperty const& p) noexcept -> bool {
            auto pad = p.left;
            if (pad.is_absolute()) return pad.i > 0;
            if (pad.is_precentage()) return pad.f > 0;
            return false;
        }
        constexpr auto has_start_whitespace() const noexcept -> bool { return has_start_whitespace(padding); }

        static constexpr auto has_end_whitespace(QuadProperty const& p) noexcept -> bool {
            auto pad = p.right;
            if (pad.is_absolute()) return pad.i > 0;
            if (pad.is_precentage()) return pad.f > 0;
            return false;
        }
        constexpr auto has_end_whitespace() const noexcept -> bool { return has_end_whitespace(padding); }

        static constexpr auto ignore_vertical_layout_shift(Display d) noexcept -> bool {
            return d == Display::Inline;
        }
        constexpr auto ignore_vertical_layout_shift() noexcept -> bool { return ignore_vertical_layout_shift(display); };

        static constexpr auto ignore_size(Display d) noexcept -> bool {
            return d == Display::Inline;
        }
        constexpr auto ignore_size() const noexcept -> bool { return ignore_size(display); };

        constexpr auto can_collapse_margin() const noexcept -> bool {
            if (item_type == ItemType::None) return true;
            return false;
        }
    private:
        static constexpr auto get_property(
            std::unordered_map<std::string_view, std::string_view> const& props,
            std::string_view key
        ) noexcept -> std::string_view {
            if (auto it = props.find(key); it != props.end()) {
                return it->second;
            } else {
                return {};
            }
        }
    };
} // namespace termml::css

#include <format>

template <>
struct std::formatter<termml::css::RGBColor> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(termml::css::RGBColor const& c, auto& ctx) const {
        auto out = ctx.out();
        std::format_to(out, "rgb({}, {}, {})", c.r, c.g, c.b);
        return out;
    }
};

template <>
struct std::formatter<termml::css::Color> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(termml::css::Color const& c, auto& ctx) const {
        auto out = ctx.out();
        if (c.is_rgb()) {
            std::format_to(out, "{}", c.as_rgb());
        } else if (c.is_8bit() || c.is_4bit()) {
            std::format_to(out, "Bit({})", int(c.as_bit()));
        } else if (c.is_transparent()) {
            std::format_to(out, "transparent");
        }
        return out;
    }
};

template <>
struct std::formatter<termml::css::Number> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(termml::css::Number const& n, auto& ctx) const {
        auto out = ctx.out();
        if (n.unit == termml::css::Unit::Percentage) {
            std::format_to(out, "{}%", n.f);
        } else if (n.unit == termml::css::Unit::Auto) {
            std::format_to(out, "fit");
        } else {
            std::format_to(out, "{}c", n.i);
        }
        return out;
    }
};

template <>
struct std::formatter<termml::css::BorderStyle> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(termml::css::BorderStyle const& v, auto& ctx) const {
        auto out = ctx.out();
        switch (v) {
            case termml::css::BorderStyle::None: return std::format_to(out, "none");
            case termml::css::BorderStyle::Solid: return std::format_to(out, "solid");
            case termml::css::BorderStyle::Dotted: return std::format_to(out, "dotted");
        }
        return out;
    }
};

template <>
struct std::formatter<termml::css::Border> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(termml::css::Border const& v, auto& ctx) const {
        auto out = ctx.out();
        std::format_to(out, "{} {} {}", v.width, v.style, v.color);
        return out;
    }
};

template <>
struct std::formatter<termml::css::BorderType> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(termml::css::BorderType const& v, auto& ctx) const {
        auto out = ctx.out();
        switch (v) {
            case termml::css::BorderType::Sharp: return std::format_to(out, "Sharp");
            case termml::css::BorderType::Rounded: return std::format_to(out, "Rounded");
        }
        return out;
    }
};

template <>
struct std::formatter<termml::css::Overflow> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(termml::css::Overflow const& v, auto& ctx) const {
        auto out = ctx.out();
        switch (v) {
            case termml::css::Overflow::Clip: return std::format_to(out, "Clip");
            case termml::css::Overflow::Auto: return std::format_to(out, "Auto");
            case termml::css::Overflow::Visible: return std::format_to(out, "Visible");
            case termml::css::Overflow::Scroll: return std::format_to(out, "Scroll");
        }
        return out;
    }
};
template <>
struct std::formatter<termml::css::QuadProperty> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(termml::css::QuadProperty const& v, auto& ctx) const {
        auto out = ctx.out();
        std::format_to(out, "QuadProperty(top: {}, right: {}, bottom: {}, left: {})", v.top, v.right, v.bottom, v.left);
        return out;
    }
};

template <>
struct std::formatter<termml::css::Display> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(termml::css::Display const& v, auto& ctx) const {
        auto out = ctx.out();
        switch (v) {
            case termml::css::Display::Block: return std::format_to(out, "Block");
            case termml::css::Display::InlineBlock: return std::format_to(out, "InlineBlock");
            case termml::css::Display::Inline: return std::format_to(out, "Inline");
            case termml::css::Display::Flex: return std::format_to(out, "Flex");
            case termml::css::Display::Grid: return std::format_to(out, "Grid");
        }
        return out;
    }
};

template <>
struct std::formatter<termml::css::Whitespace> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(termml::css::Whitespace const& v, auto& ctx) const {
        auto out = ctx.out();
        switch (v) {
            case termml::css::Whitespace::Normal: return std::format_to(out, "Normal");
            case termml::css::Whitespace::NoWrap: return std::format_to(out, "NoWrap");
            case termml::css::Whitespace::Pre: return std::format_to(out, "Pre");
            case termml::css::Whitespace::PreWrap: return std::format_to(out, "PreWrap");
            case termml::css::Whitespace::PreLine: return std::format_to(out, "PreLine");
        }
        return out;
    }
};

template <>
struct std::formatter<termml::css::Style> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(termml::css::Style const& v, auto& ctx) const {
        auto out = ctx.out();

        std::format_to(out, "Style{{");
        std::format_to(out, "min-width: {}, max-width: {}, width: {}, ", v.min_width, v.max_width, v.width);

        std::format_to(out, "min-height: {}, max-height: {}, height: {}, ", v.min_height, v.max_height, v.height);
        std::format_to(out, "display: {}, ", v.display);

        std::format_to(out, "border-top: {}, ", v.border_top);
        std::format_to(out, "border-right: {}, ", v.border_right);
        std::format_to(out, "border-bottom: {}, ", v.border_bottom);
        std::format_to(out, "border-left: {}, ", v.border_left);

        std::format_to(out, "border_type: {}, ", v.border_type);

        std::format_to(out, "padding: ({}), ", v.padding);

        std::format_to(out, "z-index: {}, ", v.z_index);
        std::format_to(out, "white-space: {}, ", v.whitespace);
        std::format_to(out, "overflow: (x: {}, y: {}), ", v.overflow_x, v.overflow_y);
        std::format_to(out, "color: {}, bg-color: {}", v.fg_color, v.bg_color);

        std::format_to(out, "}}");
        return out;
    }
};
#endif // AMT_TERMML_CSS_STYLE_HPP
