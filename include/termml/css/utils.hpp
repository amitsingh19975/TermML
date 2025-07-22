#ifndef AMT_TERMML_CSS_UTILS_HPP
#define AMT_TERMML_CSS_UTILS_HPP

#include <string_view>

namespace termml::utils {
    struct LineCharSet {
        std::string_view vertical;
        std::string_view horizonal;
        std::string_view tl_corner; // top left corner
        std::string_view tr_corner; // top right corner
        std::string_view br_corner; // bottom right corner
        std::string_view bl_corner; // bottom left corner
        std::string_view cross;
        std::string_view plus;
    };

    struct BoxCharSet {
        std::string_view vertical;
        std::string_view horizonal;
        std::string_view top_left;
        std::string_view top_right;
        std::string_view bottom_right;
        std::string_view bottom_left;
        std::string_view left_connector; // -|
        std::string_view top_connector;
        std::string_view right_connector; // |-
        std::string_view bottom_connector; // |-
    };

    struct ArrowCharSet {
        std::string_view up;
        std::string_view right;
        std::string_view down;
        std::string_view left;
    };

    namespace char_set {
        namespace line {
            static constexpr auto rounded = LineCharSet {
                .vertical = "│",
                .horizonal = "─",
                .tl_corner = "╭",
                .tr_corner = "╮",
                .br_corner = "╯",
                .bl_corner = "╰",
                .cross = "╳",
                .plus = "┽"
            };

            static constexpr auto rounded_bold = LineCharSet {
                .vertical = "┃",
                .horizonal = "━",
                .tl_corner = "┏",
                .tr_corner = "┓",
                .br_corner = "┛",
                .bl_corner = "┗",
                .cross = "╳",
                .plus = "╋"
            };

            static constexpr auto rect = LineCharSet {
                .vertical = "│",
                .horizonal = "─",
                .tl_corner = "┌",
                .tr_corner = "┐",
                .br_corner = "┘",
                .bl_corner = "└",
                .cross = "x",
                .plus = "+"
            };

            static constexpr auto rect_bold = LineCharSet {
                .vertical = "┃",
                .horizonal = "━",
                .tl_corner = "┏",
                .tr_corner = "┓",
                .br_corner = "┛",
                .bl_corner = "┗",
                .cross = "✖",
                .plus = "➕"
            };

            static constexpr auto dotted = LineCharSet {
                .vertical = "┆",
                .horizonal = "┄",
                .tl_corner = "┌",
                .tr_corner = "┐",
                .br_corner = "┘",
                .bl_corner = "└",
                .cross = "x",
                .plus = "+"
            };
            static constexpr auto dotted_bold = LineCharSet {
                .vertical = "┇",
                .horizonal = "┉",
                .tl_corner = "┏",
                .tr_corner = "┓",
                .br_corner = "┛",
                .bl_corner = "┗",
                .cross = "✖",
                .plus = "➕"
            };
        } // namespace line

        namespace box {
            static constexpr auto ascii = BoxCharSet {
                .vertical = "|",
                .horizonal = "-",
                .top_left = "",
                .top_right = "",
                .bottom_right = "",
                .bottom_left = "",
                .left_connector = "",
                .top_connector = "",
                .right_connector = "",
                .bottom_connector = "",
            };

            static constexpr auto rounded = BoxCharSet {
                .vertical = "│",
                .horizonal = "─",
                .top_left = "╭",
                .top_right = "╮",
                .bottom_right = "╯",
                .bottom_left = "╰",
                .left_connector = "┤",
                .top_connector = "┴",
                .right_connector = "├",
                .bottom_connector = "┬",
            };

            static constexpr auto rounded_bold = BoxCharSet {
                .vertical = "┃",
                .horizonal = "━",
                .top_left = "┏",
                .top_right = "┓",
                .bottom_right = "┛",
                .bottom_left = "┗",
                .left_connector = "┨",
                .top_connector = "┸",
                .right_connector = "┠",
                .bottom_connector = "┰",
            };

            static constexpr auto doubled = BoxCharSet {
                .vertical = "║",
                .horizonal = "═",
                .top_left = "╔",
                .top_right = "╗",
                .bottom_right = "╝",
                .bottom_left = "╚",
                .left_connector = "╢",
                .top_connector = "╧",
                .right_connector = "╟",
                .bottom_connector = "╤",
            };

            static constexpr auto dotted = BoxCharSet {
                .vertical = "┆",
                .horizonal = "┄",
                .top_left = "┌",
                .top_right = "┐",
                .bottom_right = "┘",
                .bottom_left = "└",
                .left_connector = "┤",
                .top_connector = "┴",
                .right_connector = "├",
                .bottom_connector = "┬",
            };
            static constexpr auto dotted_bold = BoxCharSet {
                .vertical = "┇",
                .horizonal = "┉",
                .top_left = "┏",
                .top_right = "┓",
                .bottom_right = "┛",
                .bottom_left = "┗",
                .left_connector = "┨",
                .top_connector = "┸",
                .right_connector = "┠",
                .bottom_connector = "┰",
            };
        } // namespace box

        namespace arrow {
            static constexpr auto ascii = ArrowCharSet {
                .up = "^",
                .right = ">",
                .down = "v",
                .left = "<"
            };
            static constexpr auto basic = ArrowCharSet {
                .up = "△",
                .right = "▷",
                .down = "▽",
                .left = "◁"
            };

            static constexpr auto basic_bold = ArrowCharSet {
                .up = "▲",
                .right = "▶",
                .down = "▼",
                .left = "◀"
            };
        } // namespace arrow
    } // namespace char_set
} // namespace termml::utils

#endif // AMT_TERMML_CSS_UTILS_HPP
