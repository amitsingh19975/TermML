#ifndef AMT_TERMML_LAYOUT_HPP
#define AMT_TERMML_LAYOUT_HPP

#include "style.hpp"
#include "termml/core/bounding_box.hpp"
#include "xml/node.hpp"
#include <cctype>
#include <limits>
#include <string_view>
#include <utility>
#include <vector>

namespace termml::layout {
    using node_index_t = std::size_t;

    struct LayoutNode {
        std::string_view tag{};
        node_index_t node_index{std::numeric_limits<node_index_t>::max()};
        std::size_t style_index{std::numeric_limits<node_index_t>::max()};
        std::string_view text{};
        std::vector<node_index_t> children{};
        core::BoundingBox viewport{}; // if std::nullopt, bounds are not resolved.

        unsigned scroll_x{};
        unsigned scroll_y{};

        bool scrollable_x{false};
        bool scrollable_y{false};
    };

    struct LayoutContext {
        core::BoundingBox viewport;
        std::vector<LayoutNode> nodes;
        std::vector<std::string_view> strings;

        constexpr LayoutContext(core::BoundingBox vp) noexcept
            : viewport(vp)
        {}

        LayoutContext(LayoutContext const&) = delete;
        LayoutContext(LayoutContext &&) noexcept = default;
        LayoutContext& operator=(LayoutContext const&) = delete;
        LayoutContext& operator=(LayoutContext &&) noexcept = default;
        ~LayoutContext() = default;

        auto compute(xml::Context* context) -> void {
            context->resolve_css();
            nodes.clear();
            auto layout = LayoutNode {
                .tag = {},
                .node_index = 0,
                .style_index = 0,
                .viewport = viewport
            };

            nodes.push_back(std::move(layout));
            initialize_nodes(context, { .index = 0, .kind = xml::NodeKind::Element }, 0);
            resolve_style(context);
        }

        auto dump(xml::Context const* context, node_index_t index = 0, unsigned level = 0) const -> void {
            auto tab = level * 4;
            auto const& l = nodes[index];
            std::println("{:{}} > {}", ' ', tab, l.tag);
            if(!l.text.empty()) {
                std::println("{:{}}   |- Text: '{}'", ' ', tab, l.text);
            }

            std::println("{:{}}   |- Style: [{}]", ' ', tab, context->styles[l.style_index]);

            for (auto n: l.children) {
                dump(context, n, level + 1);
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
                    .style_index = el.style_index
                });
                nodes[layout_node_index].children.push_back(next_index);
                current_node_index = next_index;
            }

            for (auto i = 0ul; i < el.childern.size(); ++i) {
                auto const& ch = el.childern[i];
                auto next_index = nodes.size();
                if (ch.kind == xml::NodeKind::TextContent) {
                    auto txt = context->text_nodes[ch.index].normalized_text;

                    nodes.push_back({
                        .tag = {},
                        .node_index = std::numeric_limits<std::size_t>::max(),
                        .style_index = context->text_nodes[ch.index].style_index,
                        .text = txt
                    });
                    nodes[layout_node_index].children.push_back(next_index);
                } else if (ch.kind == xml::NodeKind::Element) {
                    initialize_nodes(context, ch, current_node_index);
                }
            }
            return current_node_index;
        }

        auto resolve_style(xml::Context* context, node_index_t node = 0) -> void {
            auto const& layout = nodes[node];
            auto& style = context->styles[layout.style_index];

            if (node == 0) {
                style.width = style::Number {
                    .i = static_cast<int>(viewport.width),
                    .unit = style::Unit::Cell
                };

                style.height = style::Number {
                    .i = static_cast<int>(viewport.height),
                    .unit = style::Unit::Cell
                };

                style.inset = {
                    .top = style::Number::from_cell(static_cast<int>(viewport.min_y())),
                    .right = style::Number::from_cell(static_cast<int>(viewport.max_x())),
                    .bottom = style::Number::from_cell(static_cast<int>(viewport.max_y())),
                    .left = style::Number::from_cell(static_cast<int>(viewport.min_x())),
                };
            }

            if (style.width.is_absolute()) {
                for (auto l: layout.children) {
                    auto& ts = context->styles[nodes[l].style_index];
                    resolve_style_width_releated_props(ts, style.width.i);
                    resolve_style(context, l);
                }
            }

            if (style.height.is_absolute()) {
                for (auto l: layout.children) {
                    auto& ts = context->styles[nodes[l].style_index];
                    ts.min_height = ts.min_height.resolve(style.height.i);
                    ts.max_height = ts.max_height.resolve(style.height.i);
                    ts.height = ts.height.resolve(style.height.i);
                    resolve_style(context, l);
                }
            }
        }

        static constexpr auto resolve_style_width_releated_props(
            style::Style& style,
            int parent_width
        ) noexcept -> void {
            parent_width = std::max(parent_width, 0);
            style.min_width = style.min_width.resolve(parent_width);
            style.max_width = style.max_width.resolve(parent_width);
            style.width = style.width.resolve(parent_width);
            style.border_top.width = style.border_top.width.resolve(0);
            style.border_right.width = style.border_right.width.resolve(0);
            style.border_bottom.width = style.border_bottom.width.resolve(0);
            style.border_right.width = style.border_right.width.resolve(0);
            style.padding = style.padding.resolve(parent_width);
            style.margin = style.margin.resolve(parent_width);
            style.inset = style.inset.resolve(parent_width);
        }
    };

} // namespace termml::layout

#endif // AMT_TERMML_LAYOUT_HPP
