#ifndef AMT_TERMML_XML_PARSER_HPP
#define AMT_TERMML_XML_PARSER_HPP

#include "node.hpp"
#include "termml/core/string_utils.hpp"
#include "termml/xml/token.hpp"
#include <cassert>
#include <memory>
#include <print>
#include <unordered_map>

namespace termml::xml {

    struct Parser {
        std::unique_ptr<Context> context;

        Parser(Lexer&& lexer) noexcept
            : context(std::make_unique<Context>(std::move(lexer)))
        {
            context->element_nodes.push_back({
                .tag = "#root",
                .token_index = lexer.tokens.size()
            });
        }

        auto parse() -> void {
            parse_helper(0);
        };

        constexpr auto empty() const noexcept -> bool {
            return m_index >= context->lexer.tokens.size();
        }

    private:
        template <typename... Args>
            requires ((std::same_as<Args, TokenKind> && ...) && sizeof...(Args) > 0)
        auto find_next(Args... ks) -> void {
            auto const& tokens = context->lexer.tokens;
            while (!empty() && !tokens[m_index].is(ks...)) {
                ++m_index;
            }
        }

        constexpr auto current_token() const noexcept -> Token const& {
            assert(!empty());
            return context->lexer.tokens[m_index];
        }

        auto compute_string(std::string_view s) -> std::string_view {
            auto text = core::utils::trim(s);
            auto has_escape = s.find('\\') != std::string_view::npos;
            if (has_escape) {
                std::string tmp;
                tmp.reserve(text.size() + 8);
                for (auto i = 0ul; i < s.size(); ++i) {
                    auto c = s[i];
                    if (c != '\\') {
                        tmp.push_back(c);
                        continue;
                    }
                    if (i + 1 >= s.size()) break;

                    c = s[i + 1];
                    switch (c) {
                        case 'n': tmp.push_back('\n'); break;
                        case 'r': tmp.push_back('\r'); break;
                        case 't': tmp.push_back('\t'); break;
                        case 'b': tmp.push_back('\b'); break;
                        case 'f': tmp.push_back('\f'); break;
                        case 'v': tmp.push_back('\v'); break;
                        case '\\': tmp.push_back('\\'); break;
                        case '\'': tmp.push_back('\''); break;
                        case '"': tmp.push_back('"'); break;
                    }
                }

                context->computed_string.push_back(std::make_unique<std::string>(std::move(tmp)));
                text = *context->computed_string.back();
            }
            return text;
        }

        auto parse_start_tag(node_index_t node_index) -> bool {
            if (empty()) return false;
            if(!current_token().is(TokenKind::StartOpenTag)) return false;
            find_next(TokenKind::Identifier);
            if (empty()) return false;
            auto token = current_token();

            auto tag_text = token.text(context->lexer.source);
            auto& node = context->element_nodes[node_index];
            node = ElementNode {
                .tag = tag_text,
                .token_index = m_index
            };

            auto current = m_index + 1;
            find_next(TokenKind::CloseTag, TokenKind::EmptyCloseTag);
            if (empty()) return false;

            auto end = m_index;

            for (; current < end;) {
                auto const& t = context->lexer.tokens[current];
                if (t.is(TokenKind::Identifier)) {
                    auto attribute = t.text(context->lexer.source);
                    auto next = current + 1;
                    if (next >= end) {
                        node.attributes[attribute] = "";
                        break;
                    }

                    if (context->lexer.tokens[next].is(TokenKind::EqualSign)) {
                        current = next + 1;

                        if (current >= end) {
                            node.attributes[attribute] = "";
                            break;
                        }

                        if (context->lexer.tokens[current].is(TokenKind::String)) {
                            node.attributes[attribute] = compute_string(context->lexer.tokens[current].text(context->lexer.source));
                            if (attribute == "id") {
                                context->id_cache[node.attributes[attribute]] = node_index;
                            }
                            ++current;
                        }
                    }
                } else {
                    ++current;
                }
            }
            return true;
        }

        constexpr auto parse_end_tag(std::string_view tag) noexcept -> bool {
            if (empty()) return false;
            if (current_token().is(TokenKind::EmptyCloseTag)) {
                ++m_index;
                return true;
            }
            if (!current_token().is(TokenKind::EndOpenTag)) return false;

            find_next(TokenKind::Identifier);
            if (empty()) return false;

            [[maybe_unused]] auto text = current_token().text(context->lexer.source);
            assert(text == tag);

            find_next(TokenKind::CloseTag);
            if (empty()) return false;
            ++m_index;
            return true;
        }

        auto parse_element(node_index_t root_index) -> void {
            if (empty()) return;
            if (current_token().is(TokenKind::Eof)) return;

            auto token = current_token();

            auto& root = context->element_nodes[root_index];
            if (token.is(TokenKind::TextContent)) {
                auto text = current_token().text(context->lexer.source);

                root.childern.push_back({
                    .index = context->text_nodes.size(),
                    .kind = NodeKind::TextContent
                });

                context->text_nodes.push_back({
                    .token_index = m_index,
                    .text = text
                });
                ++m_index;
            } else if (token.is(TokenKind::StartOpenTag)) {
                auto node_index = context->element_nodes.size();
                root.childern.push_back({
                    .index = node_index,
                    .kind = NodeKind::Element
                });

                context->element_nodes.emplace_back();

                auto is_valid = parse_start_tag(node_index);
                if (!is_valid) return;

                auto& node = context->element_nodes[node_index];
                auto tag = node.tag;

                if (empty()) return;
                if (!current_token().is(TokenKind::EmptyCloseTag)) {
                    ++m_index;
                    while (!empty() && !current_token().is(TokenKind::EndOpenTag, TokenKind::Eof)) {
                        parse_element(node_index);
                    }
                }

                parse_end_tag(tag);
            }
        }

        auto parse_helper(node_index_t root_index) -> void {
            while (!empty()) {
                if (current_token().is(TokenKind::Eof)) return;
                parse_element(root_index);
            }
        }

        auto resolve_css_inheritance(Node const& node = xml::Context::root) -> void {
            if (node.kind != NodeKind::Element) return;

            auto& el = context->element_nodes[node.index];
            std::vector<std::string_view> to_be_removed;
            for (auto [k, v]: el.attributes) {
                if (v == "inherit") {
                    to_be_removed.push_back(k);
                }
            }

            for (auto k: to_be_removed) el.attributes.erase(k);

            for (auto ch: el.childern) {
                if (ch.kind != NodeKind::Element) continue;
                auto& child = context->element_nodes[ch.index];
                std::unordered_map<std::string_view, std::string_view> tmp; 
                for (auto [k, v]: child.attributes) {
                    if (v != "inherit") {
                        continue;
                    }

                    if (auto it = el.attributes.find(k); it != child.attributes.end()) {
                        tmp[k] = it->second;
                    }
                }

                if (!tmp.empty()) {
                    for (auto [k, v]: tmp) child.attributes[k] = v;
                }
                resolve_css_inheritance(ch);
            }
        }
    private:
        std::size_t m_index{};
    }; 

} // namespace termml::xml

#endif // AMT_TERMML_XML_PARSER_HPP
