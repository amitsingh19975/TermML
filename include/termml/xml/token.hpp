#ifndef AMT_TERMML_XML_TOKEN_HPP
#define AMT_TERMML_XML_TOKEN_HPP

#include <string_view>
namespace termml::xml {

    enum class TokenKind {
        StartOpenTag, // <
        EndOpenTag, // </
        CloseTag, // >
        EmptyCloseTag, // />
        CommentOpen, // <!--
        CommentClose, // -->

        EqualSign, // =

        Identifier, // div, span
        String,
        TextContent,

        EntityRef,
        CharRef,

        Eof,
        Error
    };

    struct Token {
        TokenKind kind;
        unsigned start;
        unsigned end;

        template <typename... Args>
            requires ((std::same_as<Args, TokenKind> && ...) && sizeof...(Args) > 0)
        constexpr auto is(Args... args) const noexcept -> bool { return ((kind == args) || ...); }
        constexpr auto eof() const noexcept -> bool { return is(TokenKind::Eof); }

        constexpr auto text(std::string_view source) const noexcept -> std::string_view {
            if (eof()) return {};
            return source.substr(start, end - start);
        }
    };

} // namespace termml::xml


#include <format>

template <>
struct std::formatter<termml::xml::TokenKind> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(termml::xml::TokenKind const& k, auto& ctx) const {
        auto out = ctx.out();
        switch (k) {
            case termml::xml::TokenKind::StartOpenTag: std::format_to(out, "StartOpenTag"); break;
            case termml::xml::TokenKind::EndOpenTag: std::format_to(out, "EndOpenTag"); break;
            case termml::xml::TokenKind::CloseTag: std::format_to(out, "CloseTag"); break;
            case termml::xml::TokenKind::EmptyCloseTag: std::format_to(out, "EmptyCloseTag"); break;
            case termml::xml::TokenKind::CommentOpen: std::format_to(out, "CommentOpen"); break;
            case termml::xml::TokenKind::CommentClose: std::format_to(out, "CommentClose"); break;
            case termml::xml::TokenKind::EqualSign: std::format_to(out, "EqualSign"); break;
            case termml::xml::TokenKind::Identifier: std::format_to(out, "Identifier"); break;
            case termml::xml::TokenKind::String: std::format_to(out, "String"); break;
            case termml::xml::TokenKind::TextContent: std::format_to(out, "TextContent"); break;
            case termml::xml::TokenKind::EntityRef: std::format_to(out, "EntityRef"); break;
            case termml::xml::TokenKind::CharRef: std::format_to(out, "CharRef"); break;
            case termml::xml::TokenKind::Eof: std::format_to(out, "Eof"); break;
            case termml::xml::TokenKind::Error: std::format_to(out, "Error"); break;
        }
        return out;
    }
};

template <>
struct std::formatter<termml::xml::Token> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(termml::xml::Token const& t, auto& ctx) const {
        auto out = ctx.out();
        std::format_to(out, "Token(kind={}, start={}, end={})", t.kind, t.start, t.end);
        return out;
    }
};

#endif // AMT_TERMML_XML_TOKEN_HPP
