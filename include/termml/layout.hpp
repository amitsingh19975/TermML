#ifndef AMT_TERMML_LAYOUT_HPP
#define AMT_TERMML_LAYOUT_HPP

#include "style.hpp"
#include "xml/node.hpp"
#include <array>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <type_traits>
#include <unordered_map>
#include <string_view>
#include <utility>
#include <vector>

namespace termml::layout {
    using node_index_t = std::size_t;

    struct BoundingBox {
        unsigned x{};
        unsigned y{};
        unsigned width{};
        unsigned height{};

        constexpr auto min_x() const noexcept -> unsigned { return x; }
        constexpr auto min_y() const noexcept -> unsigned { return y; }
        constexpr auto max_x() const noexcept -> unsigned { return x + width; }
        constexpr auto max_y() const noexcept -> unsigned { return y + height; }
    };

    struct LayoutNode {
        std::string_view tag{};
        node_index_t node_index{std::numeric_limits<node_index_t>::max()};
        std::string_view text{};
        std::vector<node_index_t> children{};
        BoundingBox viewport{}; // if std::nullopt, bounds are not resolved.

        unsigned scroll_x{};
        unsigned scroll_y{};

        bool scrollable_x{false};
        bool scrollable_y{false};
    };

    struct LayoutContext {
        BoundingBox viewport;
        std::vector<LayoutNode> nodes;
        std::vector<std::string_view> strings;

        constexpr LayoutContext(BoundingBox vp) noexcept
            : viewport(vp)
        {}

        LayoutContext(LayoutContext const&) = delete;
        LayoutContext(LayoutContext &&) noexcept = default;
        LayoutContext& operator=(LayoutContext const&) = delete;
        LayoutContext& operator=(LayoutContext &&) noexcept = default;
        ~LayoutContext() = default;

        auto compute(xml::Context const* context) -> void {
            nodes.clear();
            auto layout = LayoutNode {
                .tag = {},
                .node_index = 0,
                .viewport = viewport
            };

            nodes.push_back(std::move(layout));
            initialize_nodes(context, { .index = 0, .kind = xml::NodeKind::Element }, 0);
        }

        auto dump(node_index_t index = 0, unsigned level = 0) const -> void {
            auto tab = level * 4;
            auto const& l = nodes[index];
            std::println("{:{}} > {}", ' ', tab, l.tag);
            // std::println("{:{}}   |- {}", ' ', tab, style);
            if(!l.text.empty()) {
                std::println("{:{}}   |- Text: '{}'", ' ', tab, l.text);
            }

            for (auto n: l.children) {
                dump(n, level + 1);
            }
        }
    private:
        auto initialize_nodes(xml::Context const* context, xml::Node const& node, node_index_t layout_node_index) -> node_index_t /*inserted layout_index*/ {
            if (node.kind == xml::NodeKind::TextContent) {
                std::unreachable();
            }

            auto const& el = context->element_nodes[node.index];

            auto current_node_index = layout_node_index;
            if (node.index != 0) {
                auto next_index = nodes.size();
                nodes.push_back({
                    .tag = el.tag,
                    .node_index = node.index,
                });
                nodes[layout_node_index].children.push_back(next_index);
                current_node_index = next_index;
            }

            for (auto i = 0ul; i < el.childern.size(); ++i) {
                auto const& ch = el.childern[i];
                auto next_index = nodes.size();
                if (ch.kind == xml::NodeKind::TextContent) {
                    auto txt = context->text_nodes[ch.index].text;

                    nodes.push_back({
                        .tag = {},
                        .node_index = std::numeric_limits<std::size_t>::max(),
                        .text = txt
                    });
                    nodes[layout_node_index].children.push_back(next_index);
                } else {
                    initialize_nodes(context, ch, current_node_index);
                }
            }
            return current_node_index;
        }
    };

} // namespace termml::layout

#include <format>

template <>
struct std::formatter<termml::layout::BoundingBox> {
    constexpr auto parse(auto& ctx) {
        auto it = ctx.begin();
        while (it != ctx.end()) {
            if (*it == '}') break;
            ++it;
        }
        return it;
    }

    auto format(termml::layout::BoundingBox const& v, auto& ctx) const {
        auto out = ctx.out();
        std::format_to(out, "BoundingBox(x: {}, y: {}, width: {}, height: {})", v.x, v.y, v.width, v.height);
        return out;
    }
};


#endif // AMT_TERMML_LAYOUT_HPP
