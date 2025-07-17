#ifndef AMT_TERMML_LAYOUT_HPP
#define AMT_TERMML_LAYOUT_HPP

#include "style.hpp"
#include "core/bounding_box.hpp"
#include "text.hpp"
#include "xml/node.hpp"
#include <algorithm>
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
            resolve_cyclic_width(context, 0, viewport.width);
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
                    nodes[current_node_index].children.push_back(next_index);
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
                    resolve_style_height_releated_props(ts, style.height.i);
                    resolve_style(context, l);
                }
            }
        }

        static constexpr auto resolve_style_width_releated_props(
            style::Style& style,
            int parent_width,
            bool resolve_auto_fit = false
        ) noexcept -> void {
            parent_width = std::max(parent_width, 0);

            if (resolve_auto_fit) {
                style.width = style.width.resolve_all(parent_width);
                style.min_width = style.min_width.resolve_all(parent_width);
                style.max_width = style.max_width.resolve_all(parent_width);
                style.width.i = std::max(style.width.i, style.min_width.i);
                if (style.overflow_x == style::Overflow::Clip) {
                    style.width.i = std::min(style.width.i, style.max_width.i);
                }
            } else {
                style.width = style.width.resolve_percentage(parent_width);
                style.min_width = style.min_width.resolve_percentage(parent_width);
                style.max_width = style.max_width.resolve_percentage(parent_width);
            }

            style.padding = style.padding.resolve(parent_width);
            style.margin = style.margin.resolve(parent_width);
            style.inset = style.inset.resolve(parent_width);
        }

        static constexpr auto resolve_style_height_releated_props(
            style::Style& style,
            int parent_height,
            bool resolve_auto_fit = false
        ) noexcept -> void {
            if (resolve_auto_fit) {
                style.min_height = style.min_height.resolve_all(parent_height);
                style.max_height = style.max_height.resolve_all(parent_height);
                style.height = style.height.resolve_all(parent_height);
                style.height.i = std::max(style.height.i, style.min_height.i);
                if (style.overflow_y == style::Overflow::Clip) {
                    style.height.i = std::min(style.height.i, style.max_height.i);
                }
            } else {
                style.min_height = style.min_height.resolve_percentage(parent_height);
                style.max_height = style.max_height.resolve_percentage(parent_height);
                style.height = style.height.resolve_percentage(parent_height);
            }
        }

        constexpr auto resolve_cyclic_width(
            xml::Context* context,
            node_index_t node,
            int max_parent_width,
            int level = 0
        ) noexcept -> int /*container width*/ {
            auto const& el = nodes[node];
            auto& style = context->styles[el.style_index];
            auto content_width = 0;

            if (el.tag.empty() && node != 0) {
                auto text = text::TextRenderer{
                    .text = el.text
                };
                content_width = std::min(text.measure_width(), max_parent_width);
            }

            for (auto l: el.children) {
                auto const& c = nodes[l];
                auto& cs = context->styles[c.style_index];
                if (cs.width.is_absolute()) {
                    content_width = std::max(content_width, cs.width.i);
                    resolve_cyclic_width(context, l, cs.width.i, level + 1);
                } else if (cs.width.is_fit()) {
                    auto parent_width = style.width.is_absolute() ? style.width.i : max_parent_width;
                    content_width = std::max(
                        content_width,
                        resolve_cyclic_width(context, l, parent_width, level + 1)
                    );
                } else if (cs.width.is_precentage()) {
                    // cannot resolve this so we set this to 0
                    resolve_style_width_releated_props(cs, 0, true);
                }
            }

            auto& padding = style.padding;
            auto& border_left = style.border_left;
            auto& border_right = style.border_right;

            content_width += border_left.border_width();
            content_width += border_right.border_width();

            auto per = 0.f;
            if (padding.left.is_precentage()) per += padding.left.f / 100.f;
            if (padding.right.is_precentage()) per += padding.right.f / 100.f;
            per = 1.f - per;

            auto actual_width = content_width;

            if (per >= 0.0001f) {
                actual_width = static_cast<int>(float(content_width) / per);
            }

            resolve_style_width_releated_props(style, actual_width, true);

            return actual_width;
        }
    };

} // namespace termml::layout

#endif // AMT_TERMML_LAYOUT_HPP
