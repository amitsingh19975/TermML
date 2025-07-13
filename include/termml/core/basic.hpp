#ifndef AMT_TERMML_CORE_BASIC_HPP
#define AMT_TERMML_CORE_BASIC_HPP

#include <string_view>

namespace termml::core {
    static constexpr std::string_view escape_seq_start = "\x1b[";

    constexpr auto trim_escape_seq(std::string_view code) noexcept -> std::string_view {
        if (!code.starts_with(escape_seq_start)) return code;
        return code.substr(escape_seq_start.size());
    }
} // namespace termml::core;

#endif // AMT_TERMML_CORE_BASIC_HPP
