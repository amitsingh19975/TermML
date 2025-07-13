#ifndef AMT_TERMML_CORE_UTF8_HPP
#define AMT_TERMML_CORE_UTF8_HPP

#include <array>
#include <string_view>

namespace termml::core::utf8 {
    static constexpr std::array<std::uint8_t, 16> lookup {
        1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 2, 2, 3, 4
    };

    constexpr auto get_length(char c) -> std::uint8_t {
        auto const byte = static_cast<std::uint8_t>(c);
        return lookup[byte >> 4];
    }

    constexpr auto calculate_size(std::string_view str) -> std::size_t {
        auto size = std::size_t{};
        for (auto i = 0ul; i < str.size(); ) {
            auto len = get_length(str[i]);
            i += len;
            ++size;
        }
        return size;
    }
} // namespace termml::core::utf8

#endif // AMT_TERMML_CORE_UTF8_HPP
