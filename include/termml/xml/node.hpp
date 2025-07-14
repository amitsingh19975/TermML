#ifndef AMT_TERMML_XML_NODE_HPP
#define AMT_TERMML_XML_NODE_HPP

#include "lexer.hpp"
#include "termml/core/string_utils.hpp"
#include "termml/style.hpp"
#include <cctype>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <string_view>
#include <vector>

namespace termml::xml {

    using node_index_t = std::size_t;

    enum class NodeKind {
        Element,
        TextContent
    };

    struct Node {
        node_index_t index;
        NodeKind kind;
    };

    struct ElementNode {
        using node_index_t = std::size_t;
        std::string_view tag;
        std::size_t token_index;
        std::unordered_map<std::string_view, std::string_view> attributes{};
        // Index into global node pool
        std::vector<Node> childern{};
        std::size_t style_index{};
    };

    struct TextContentNode {
        std::size_t token_index;
        std::string_view text;
        std::string_view normalized_text{};
        std::size_t style_index{};
    };

    struct StyleNode {
        std::size_t token_index;
        std::string_view text;
        // TODO: add parsed style nodes
    };

    enum class VisitorState {
        Continue,
        Break
    };

    struct Context {
        Lexer lexer;
        static constexpr Node root { .index = 0, .kind = NodeKind::Element };
        std::vector<ElementNode> element_nodes{};
        std::vector<TextContentNode> text_nodes{};
        std::vector<StyleNode> stylesNodes{};
        std::unordered_map<std::string_view, node_index_t> id_cache{};
        std::vector<style::Style> styles{};

        // unique pointer is used to stablize the string address.
        std::vector<std::unique_ptr<std::string>> computed_string{};
        std::vector<std::unique_ptr<std::string>> text_node_computed_string{};

        auto dump(Node const& node = root, unsigned level = 0) const -> void {
            auto tab = level * 4;
            if (node.kind == NodeKind::TextContent) {
                auto const& text = text_nodes[node.index];
                std::println("{:{}}#text: \"{}\"", ' ', tab, text.text);
                std::println("{:{}}#computed_text: \"{}\"", ' ', tab, text.normalized_text);
            } else if (node.kind == NodeKind::Element) {
                auto const& el = element_nodes[node.index];
                std::println("{:{}} > {}", ' ', tab, el.tag);
                std::println("{:{}}   |- Attr: {}", ' ', tab, el.attributes);
                for (auto n: el.childern) {
                    dump(n, level + 1);
                }
            }
        }

        auto dump_xml(Node const& node = root, unsigned level = 0) const -> void {
            auto tab = level * 4;
            if (node.kind == NodeKind::TextContent) {
                auto const& text = text_nodes[node.index];
                if (text.normalized_text.empty()) return;
                auto const& style = styles[text.style_index];
                if (style.display == style::Display::Block) {
                    std::println("{:{}}<#block>{}</#block>", ' ', tab, text.normalized_text);
                } else {
                    std::println("{:{}}{}", ' ', tab, text.normalized_text);
                }
            } else if (node.kind == NodeKind::Element) {
                auto const& el = element_nodes[node.index];
                std::print("{:{}} <{} ", ' ', tab, el.tag);
                for (auto [k, v]: el.attributes) {
                    std::print("{}=\"{}\" ", k, v);
                }
                if (el.style_index < styles.size()) {
                    std::print("style=\"{}\"", styles[el.style_index]);
                }
                if (el.childern.empty()) {
                    std::println("/>");
                } else {
                    std::println(">");
                    for (auto n: el.childern) {
                        dump_xml(n, level + 1);
                    }
                    std::println("{:{}} </{}> ", ' ', tab, el.tag);
                }
            }
        }

        template <typename F>
            requires (std::is_invocable_r_v<VisitorState, F, Node>)
        constexpr auto visit(F&& fn) noexcept -> void {
            visit_helper(std::forward<F>(fn), Node { .index = 0, .kind = NodeKind::Element });
        }

        auto resolve_css() {
            text_node_computed_string.clear();
            for (auto& el: text_nodes) el.normalized_text = {};

            resolve_css_inheritance();
            styles.push_back({
                .width = { .f = 100, .unit = style::Unit::Percentage },
                .height = { .f = 100, .unit = style::Unit::Percentage },
            });
            build_style_tree();
            collapse_whitespace();
        }
    private:
        template <typename F>
            requires (std::is_invocable_r_v<VisitorState, F, Node>)
        constexpr auto visit_helper(F&& fn, Node const& node) noexcept -> VisitorState {
            if (node.kind == NodeKind::TextContent) {
                return fn(element_nodes[node.index]);
            } else if (node.kind == NodeKind::Element) {
                auto const& el = element_nodes[node.index];
                if (fn(el) == VisitorState::Break) return VisitorState::Break;
                for (auto n: el.childern) {
                    if (visit_helper(fn, n) == VisitorState::Break) return VisitorState::Break;
                }
            }
            return VisitorState::Continue;
        }

        auto resolve_css_inheritance(Node const& node = xml::Context::root) -> void {
            if (node.kind != NodeKind::Element) return;

            auto& el = element_nodes[node.index];
            std::vector<std::string_view> to_be_removed;
            for (auto [k, v]: el.attributes) {
                if (v == "inherit") {
                    to_be_removed.push_back(k);
                }
            }

            for (auto k: to_be_removed) el.attributes.erase(k);

            for (auto ch: el.childern) {
                if (ch.kind != NodeKind::Element) continue;
                auto& child = element_nodes[ch.index];
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

                for (auto k: style::CSSPropertyKey::inherited_properties) {
                    if (child.attributes.contains(k)) continue;
                    if (auto it = el.attributes.find(k); it != child.attributes.end()) {
                        child.attributes[k] = it->second;
                    }
                }

                resolve_css_inheritance(ch);
            }
        }

        auto build_style_tree(Node const& node = root) -> void {
            auto const& el = element_nodes[node.index];
            for (auto c: el.childern) {
                if (c.kind == NodeKind::TextContent) {
                    auto& ch = text_nodes[c.index];
                    ch.style_index = styles.size();
                    styles.push_back(styles[el.style_index]);
                } else if (c.kind == NodeKind::Element) {
                    auto style = style::Style{};
                    auto& ch = element_nodes[c.index];
                    ch.style_index = styles.size();
                    style.parse_proprties(ch.tag, ch.attributes, &styles[el.style_index]);
                    styles.push_back(std::move(style));
                    build_style_tree(c);
                }
            }
        }

        auto normalize_text(std::string_view text, style::Whitespace whitespace, bool& allocated) -> std::string_view {
            if (text.empty()) return {};
            if (whitespace == style::Whitespace::Pre || whitespace == style::Whitespace::PreWrap) {
                return text;
            }

            auto start = text.find_first_not_of(' ');
            if (start == std::string_view::npos) start = 0;
            start = std::min(start, text.size());

            std::size_t end = text.size();
            if (whitespace == style::Whitespace::PreLine) {
                end = std::min(std::min(text.find_last_not_of(" \t\r\f\v"), text.size()) + 1, text.size());
            } else if (whitespace != style::Whitespace::Normal) {
                end = std::min(std::min(text.find_last_not_of(" \n\t\r\f\v"), text.size()) + 1, text.size());
            }

            auto need_normalization{false};
            for (auto i = start; i < end; ++i) {
                auto c = text[i];
                if ((c == '\n') && whitespace != style::Whitespace::PreLine) {
                    need_normalization = true;
                    break;
                }

                if (c == '\t' || c == '\r') {
                    need_normalization = true;
                    break;
                }

                if (i + 1 >= end) {
                    continue;
                }

                if (c == ' ' && text[i + 1] == ' ') {
                    need_normalization = true;
                    break;
                }
            }

            if (!need_normalization) return text.substr(0, end);

            if (core::utils::trim(text).empty()) {
                return " ";
            }

            text_node_computed_string.push_back(std::make_unique<std::string>());
            allocated = true;
            std::string& tmp = *text_node_computed_string.back();
            tmp.reserve(end - start + 1);

            // Keep leading and trailing whitespaces
            if (start > 0) {
                tmp.push_back(' ');
            }

            for (auto i = start; i < end;) {
                auto c = text[i];
                if ((c == '\n') && whitespace == style::Whitespace::PreLine) {
                    tmp.push_back(text[i]);
                    ++i;
                    continue;
                }
                if (c == '\r') {
                    ++i;
                    continue;
                }
                if (std::isspace(c)) {
                    tmp.push_back(' ');
                    while (i < end && std::isspace(text[i])) {
                        ++i;
                    }
                    continue;
                }
                tmp.push_back(text[i]);
                ++i;
            }

            return tmp;
        }

        auto collapse_whitespace(
            Node const& node = root,
            bool inline_context = false,
            bool last_char_was_whitespace = false
        ) -> bool {
            auto const& el = element_nodes[node.index];


            for (auto c: el.childern) {
                if (c.kind == NodeKind::TextContent) {
                    auto& ch = text_nodes[c.index];
                    auto& style = styles[ch.style_index];
                    auto txt = ch.text;
                    bool allocated{false};
                    txt = normalize_text(txt, style.whitespace, allocated);

                    std::string_view pattern = " \n\t\r\f\v";
                    if (style.whitespace == style::Whitespace::PreLine) {
                        pattern = " \t\r\f\v";
                    }

                    if (!inline_context) {
                        style.display = style::Display::Block;
                        if (core::utils::trim(txt).empty()) {
                            if (allocated) text_node_computed_string.pop_back();
                            ch.normalized_text = {};
                            continue;
                        }
                    } else {
                        style.display = style::Display::Inline;
                    }

                    if (txt.empty()) continue;
                    auto has_trailing_space = txt.back() == ' ';

                    if (last_char_was_whitespace) {
                        txt = core::utils::ltrim(txt, pattern);
                    }

                    if (!inline_context) {
                        ch.normalized_text = core::utils::trim(txt, pattern);
                        last_char_was_whitespace = false;
                    } else {
                        ch.normalized_text = txt;
                        last_char_was_whitespace = has_trailing_space;
                    }

                } else if (c.kind == NodeKind::Element) {
                    auto& ch = element_nodes[c.index];
                    auto display = styles[ch.style_index].display;
                    auto const is_inline = (display == style::Display::Inline || display == style::Display::InlineBlock);
                    last_char_was_whitespace = collapse_whitespace(c, is_inline, last_char_was_whitespace);
                }
            }

            return last_char_was_whitespace;
        }
    };

} // namespace termml::xml

#endif // AMT_TERMML_XML_NODE_HPP
