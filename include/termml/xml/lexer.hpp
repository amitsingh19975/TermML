#ifndef AMT_TERMML_XML_LEXER_HPP
#define AMT_TERMML_XML_LEXER_HPP

#include "token.hpp"
#include <cassert>
#include <cctype>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace termml::xml {

    struct Lexer {
        std::string source;
        std::string path;
        std::vector<Token> tokens;

        Lexer(std::string_view p)
            : path(p)
        {
            auto file = std::ifstream(path);
            if (!file) {
                throw std::runtime_error(std::format("file not found: {}", path));
            }

            source = std::string(
                std::istreambuf_iterator<char>(file),
                std::istreambuf_iterator<char>()
            );

            file.close();
        }

        Lexer(std::string_view s, std::string_view p)
            : source(s)
            , path(p)
        {}

        Lexer(Lexer const&) = delete;
        Lexer(Lexer &&) noexcept = default;
        Lexer& operator=(Lexer const&) = delete;
        Lexer& operator=(Lexer &&) noexcept = default;
        ~Lexer() noexcept = default;

        constexpr auto is_eof() const noexcept -> bool {
            return source.size() <= m_cursor;
        }

        constexpr auto peek() const noexcept -> char {
            if (source.size() <= m_cursor + 1) return 0;
            return source[m_cursor + 1];
        }

        auto skip_whitespace() noexcept -> void {
            while (!is_eof() && std::isspace(source[m_cursor])) {
                ++m_cursor;
            }
        }

        constexpr auto store_state() noexcept -> void {
            m_stored_state = m_cursor;
        }

        constexpr auto restore_state() noexcept -> void {
            m_cursor = m_stored_state;
        }

        auto parse_content() -> bool {
            auto start = m_cursor;

            while (!is_eof() && source[m_cursor] != '<') {
                ++m_cursor;
            }

            if (m_cursor == start) return true;


            auto text = std::string_view(source);
            auto content = text.substr(start, m_cursor - start);
            if (!content.contains('&')) {
                tokens.push_back({
                    .kind = TokenKind::TextContent,
                    .start = start,
                    .end = m_cursor
                });
                return true;
            }

            auto offset = start;

            start = 0;
            while (!content.empty()) {
                auto i = 0u;
                for (; i < content.size(); ++i) {
                    auto c = content[i];
                    if (c == '&') {
                        if (content.find(';', i + 1) == std::string_view::npos) {
                            i = static_cast<unsigned>(content.size());
                        }
                        break;
                    }
                }

                tokens.push_back({
                    .kind = TokenKind::TextContent,
                    .start = offset + start,
                    .end = offset + start + i
                });

                if (i == content.size()) break;
                content = content.substr(i);
                start = i + 1;

                auto kind = TokenKind::EntityRef;

                if (content[start] == '#') {
                    ++start;
                    kind = TokenKind::CharRef;
                }

                auto end = content.find(';', start);
                assert(end != std::string_view::npos);
                tokens.push_back({
                    .kind = kind,
                    .start = offset + start,
                    .end = offset + static_cast<unsigned>(end)
                });
                start = static_cast<unsigned>(end) + 1;
                content = content.substr(start);
                offset += start;
            }

            return true;
        }

        auto parse_string() -> void {
            ++m_cursor;
            auto start = m_cursor;
            while (!is_eof() && source[m_cursor] != '"') {
                auto c = source[m_cursor];
                if (c == '\\') m_cursor += 2;
                else ++m_cursor;
            }

            assert(source[m_cursor] == '"');

            tokens.push_back({
                .kind = TokenKind::String,
                .start = start,
                .end = m_cursor
            });
        }
    
        constexpr auto is_identifier(char c) const noexcept -> bool {
            return !(std::isspace(c) || c == '=' || c == '/' || c == '>' || c == '<' || c == '"');
        }

        auto parse_identifier() -> bool {
            if (is_eof()) return false;
            if (!is_identifier(source[m_cursor])) return false;
            auto start = m_cursor;
            while (is_identifier(source[m_cursor])) {
                ++m_cursor;
            }

            tokens.push_back({
                .kind = TokenKind::Identifier,
                .start = start,
                .end = m_cursor
            });
            return true;
        }

        auto lex() -> bool {
            bool inside_tag = false;
            while (!is_eof()) {
                skip_whitespace();
                if (is_eof()) break;

                auto c = source[m_cursor];

                if (inside_tag && parse_identifier()) {
                    continue;
                }

                switch (c) {
                    case '<': {
                        if (peek() == '/') {
                            tokens.push_back({
                                .kind = TokenKind::EndOpenTag,
                                .start = m_cursor,
                                .end = m_cursor + 2
                            });
                            ++m_cursor;
                            inside_tag = true;
                            break;
                        } else if (peek() == '!') {
                            // consume all the tokens upcoming tokens and do not generate error.
                            // We do not support "<!HTML ...>" so anything "<!..." is invalid.
                            auto start = m_cursor;
                            ++m_cursor;
                            if (peek() == '-') {
                                ++m_cursor;
                                if (peek() == '-') {
                                    tokens.push_back({
                                        .kind = TokenKind::CommentOpen,
                                        .start = start,
                                        .end = start + 3
                                    });
                                }
                            }

                            m_cursor += 2;
                            break;
                        }
                        inside_tag = true;
                        tokens.push_back({
                            .kind = TokenKind::StartOpenTag,
                            .start = m_cursor,
                            .end = m_cursor + 1
                        });
                    } break;
                    case '-': {
                        auto start = m_cursor;
                        if (peek() == '-') {
                            ++m_cursor;
                            if (peek() == '>') {
                                tokens.push_back({
                                    .kind = TokenKind::CommentClose,
                                    .start = start,
                                    .end = start + 3
                                });
                            }
                        }

                    } break;
                    case '>': {
                        inside_tag = false;
                        tokens.push_back({
                            .kind = TokenKind::CloseTag,
                            .start = m_cursor,
                            .end = m_cursor + 1
                        });
                        ++m_cursor;
                        if (parse_content()) continue;
                    } break;
                    case '/': {
                        if (peek() == '>') {
                            tokens.push_back({
                                .kind = TokenKind::EmptyCloseTag,
                                .start = m_cursor,
                                .end = m_cursor + 2
                            });
                            ++m_cursor;
                        }
                    } break;
                    case '"': {
                        parse_string();
                    } break;
                    case '=': {
                        tokens.push_back({
                            .kind = TokenKind::EqualSign,
                            .start = m_cursor,
                            .end = m_cursor + 1
                        });
                    }
                    default: {
                        if (!inside_tag) {
                            parse_content();
                            continue;
                        }
                    }
                }

                ++m_cursor;
            }

            tokens.push_back({
                .kind = TokenKind::Eof,
                .start = static_cast<unsigned>(source.size()),
                .end = static_cast<unsigned>(source.size())
            });

            while (!tokens.empty()) {
                auto& tok = tokens.back();
                auto txt = tok.text(source);
                if (txt.empty()) {
                    tokens.pop_back();
                    continue;
                }
                auto i = 0ul;
                for (auto c: txt) {
                    if (std::isspace(c)) ++i;
                }
                if (i == txt.size()) {
                    tokens.pop_back();
                } else {
                    break;
                }
            }

            return true;
        }
    private:
        unsigned m_cursor{};
        unsigned m_stored_state{};
    };

} // namespace termml::xml

#endif // AMT_TERMML_XML_LEXER_HPP
