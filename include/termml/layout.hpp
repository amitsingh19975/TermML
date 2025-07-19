#ifndef AMT_TERMML_LAYOUT_HPP
#define AMT_TERMML_LAYOUT_HPP

#include "style.hpp"
#include "core/bounding_box.hpp"
#include "core/point.hpp"
#include "core/terminal.hpp"
#include "core/device.hpp"
#include "core/string_utils.hpp"
#include "utils.hpp"
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
        core::BoundingBox container{};
        core::BoundingBox viewport{}; // if std::nullopt, bounds are not resolved.
        int content_offset_x{};
        int content_offset_y{};
        bool start_from_newline{true};
        bool trim_end_whitespace{false};

        core::Terminal canvas{};

        constexpr auto is_scrollable_x() const noexcept -> bool {
            return viewport.width < container.width;
        }

        constexpr auto is_scrollable_y() const noexcept -> bool {
            return viewport.height < container.height;
        }

        constexpr auto shift_y(int offset) noexcept -> void {
            container.y += offset;
            viewport.y += offset;
        }
    };

    struct LayoutContext {
        core::BoundingBox viewport;
        std::vector<LayoutNode> nodes;

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
            resolve_cyclic_height(context, 0, viewport.width, {});
            compute_layout(context, viewport);
        }

        template <core::detail::IsScreen S>
        auto render(core::Device<S>& dev, xml::Context const* context, node_index_t node = 0) -> void {
            render_node(dev, context, node);
        }

        auto dump(xml::Context const* context, node_index_t index = 0, unsigned level = 0) const -> void {
            auto tab = level * 4;
            auto const& l = nodes[index];
            std::println("{:{}} > {}", ' ', tab, l.tag);
            if(l.tag.empty() && index != 0) {
                std::println("{:{}}   |- Text: '{}'", ' ', tab, l.text);
            }

            std::println("{:{}}   |- Viewport: {}, Container: {} | Offset: ({}, {}) | {}", ' ', tab, l.viewport, l.container, l.content_offset_x, l.content_offset_y, l.start_from_newline);
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
                    if (txt.empty()) continue;

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
                    ts.margin = ts.margin.resolve(style.width.i);
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
            style.inset = style.inset.resolve(parent_width);
        }

        constexpr auto resolve_cyclic_width(
            xml::Context* context,
            node_index_t node,
            int max_parent_width,
            int right_margin = 0,
            bool is_next_element_inline = false
        ) noexcept -> int /*container width*/ {
            auto& el = nodes[node];
            auto& style = context->styles[el.style_index];
            auto content_width = 0;

            if (el.tag.empty() && node != 0) {
                if (style.whitespace != style::Whitespace::Normal) {
                    is_next_element_inline = false;
                }

                el.trim_end_whitespace = !is_next_element_inline;
                auto text = text::TextRenderer{
                    .text = el.trim_end_whitespace == false ? el.text : core::utils::rtrim(el.text),
                };
                content_width = std::min(text.measure_width(), max_parent_width);
            }

            for (auto i = 0ul; i < el.children.size(); ++i) {
                auto l = el.children[i];
                auto const& c = nodes[l];
                auto& cs = context->styles[c.style_index];
                cs.margin = cs.margin.resolve(max_parent_width);

                if (i + 1 < el.children.size()) {
                    auto const& nc = nodes[el.children[i + 1]];
                    auto const& ns = context->styles[nc.style_index];
                    is_next_element_inline = (ns.display == style::Display::Inline || ns.display == style::Display::InlineBlock) && (cs.display == style::Display::Inline || cs.display == style::Display::InlineBlock);
                }

                if (cs.width.is_absolute()) {
                    content_width = std::max(content_width, cs.width.i);
                    resolve_cyclic_width(context, l, cs.width.i, right_margin, is_next_element_inline);
                } else if (cs.width.is_fit()) {
                    auto parent_width = style.width.is_absolute() ? style.width.i : max_parent_width;
                    content_width = std::max(
                        content_width,
                        resolve_cyclic_width(context, l, parent_width, right_margin, is_next_element_inline)
                    );
                } else if (cs.width.is_precentage()) {
                    // cannot resolve this so we set this to 0
                    resolve_style_width_releated_props(cs, 0, true);
                }

                // TODO: support negative margins
                content_width += std::max(std::abs(right_margin - cs.margin.left.as_cell()), 0);
                right_margin = cs.margin.right.as_cell();
            }

            auto& padding = style.padding;
            auto& border_left = style.border_left;
            auto& border_right = style.border_right;

            content_width += border_left.border_width();
            content_width += border_right.border_width();

            auto per = 0.f;
            if (padding.left.is_precentage()) per += padding.left.f / 100.f;
            else content_width += padding.left.as_cell();

            if (padding.right.is_precentage()) per += padding.right.f / 100.f;
            else content_width += padding.right.as_cell();

            per = 1.f - per;

            auto actual_width = content_width;

            if (per >= 0.0001f) {
                actual_width = static_cast<int>(float(content_width) / per);
            }

            resolve_style_width_releated_props(style, actual_width, true);

            return actual_width;
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

        constexpr auto get_inline_sibling_word_size_helper(
            xml::Context* context,
            node_index_t node
        ) const noexcept -> std::pair<std::size_t /*size*/, bool /*whitespace found*/>{
            auto const& el = nodes[node];
            auto const& s = context->styles[el.style_index];
            if (s.display != style::Display::Inline) return { 0, true };

            if (el.tag.empty()) {
                auto pos = text::detail::find_word(el.text);
                if (pos >= el.text.size()) return { el.text.size(), false };
                return { pos, true };
            }

            std::size_t res{};
            for (auto l : el.children) {
                auto [sz, ws] = get_inline_sibling_word_size_helper(context, l);
                res += sz;
                if (ws) return { res, true };
            }

            return { res, false };
        }

        constexpr auto get_inline_sibling_word_size(
            xml::Context* context,
            node_index_t parent,
            std::size_t child_index
        ) const noexcept -> std::size_t {
            auto const& el = nodes[parent];
            ++child_index;
            std::size_t res{};
            for (; child_index < el.children.size(); ++child_index) {
                auto [sz, ws] = get_inline_sibling_word_size_helper(context, child_index);
                res += sz;
                if (ws) return res;
            }

            return res;
        }

        struct HeightResult {
            int height{};
            std::size_t sibling_word_size{};
            core::Point start_offset{};
            std::pair<int, int> v_padding{};
            std::pair<int, int> v_margin{};
        };

        constexpr auto resolve_cyclic_height(
            xml::Context* context,
            node_index_t node,
            int max_parent_height,
            HeightResult param
        ) noexcept -> HeightResult {
            auto& el = nodes[node];
            auto& style = context->styles[el.style_index];
            auto content_height = 0;

            auto last_y_pos = param.start_offset.y;
            auto last_line_margin = param.v_margin;

            if (style.display != style::Display::Inline) {
                ++param.start_offset.y;
                param.start_offset.x = 0;
                content_height += 1;
            }

            if (el.tag.empty() && node != 0) {
                auto text = text::TextRenderer{
                    .text = el.text,
                    .start_offset = param.start_offset,
                    .next_word_length = param.sibling_word_size
                };
                auto tmp = text.measure_height(style);
                content_height = std::min(std::max(tmp, content_height), max_parent_height);
                param.start_offset = text.start_offset;
            }

            auto top_margin = 0;
            auto previous_block_end_margin = 0;

            for (auto i = 0ul; i < el.children.size(); ++i) {
                auto l = el.children[i];
                auto& c = nodes[l];
                auto& cs = context->styles[c.style_index];
                if (cs.display == style::Display::Block) {
                    c.start_from_newline = true;
                }
                auto sib_space = c.start_from_newline ? 0 : get_inline_sibling_word_size(context, node, i);
                param.sibling_word_size = sib_space;
                auto [pt, pb] = param.v_padding;

                auto tmp = HeightResult();

                param.start_offset.x += std::abs(previous_block_end_margin - cs.margin.left.as_cell());

                if (cs.height.is_absolute()) {
                    content_height = std::max(content_height, cs.height.i);
                    tmp = resolve_cyclic_height(
                        context,
                        l,
                        cs.height.i,
                        param
                    );
                    tmp.height = cs.height.i;
                } else if (cs.height.is_fit()) {
                    auto parent_width = style.height.is_absolute() ? style.height.i : max_parent_height;
                    tmp = resolve_cyclic_height(context, l, parent_width, param);
                } else if (cs.height.is_precentage()) {
                    // cannot resolve this so we set this to 0
                    resolve_style_height_releated_props(cs, 0, true);
                    tmp = param;
                }

                c.start_from_newline |= (tmp.start_offset.y != param.start_offset.y);

                if (cs.display == style::Display::Block) {
                    c.start_from_newline = true;
                }

                if (c.children.size() == 1) {
                    auto const& t = nodes[c.children[0]];
                    c.start_from_newline |= t.start_from_newline;
                }

                if (last_y_pos == tmp.start_offset.y) {
                    param.v_padding = {
                        std::max(tmp.v_padding.first, pt),
                        std::max(tmp.v_padding.second, pb),
                    };
                    param.v_margin = {
                        std::max(tmp.v_padding.first, param.v_margin.first),
                        std::max(tmp.v_padding.second, param.v_margin.second),
                    };
                } else {
                    tmp.height += param.v_padding.second;
                    tmp.height += std::abs(param.v_margin.first - top_margin);

                    top_margin = param.v_margin.second;
                    param.v_padding = {};
                    param.v_margin = {};
                }
                last_y_pos = tmp.start_offset.y;
                content_height = content_height + tmp.height;
                previous_block_end_margin = cs.margin.right.as_cell();
            }

            auto& padding = style.padding;
            auto& border_top = style.border_top;
            auto& border_bottom = style.border_bottom;

            content_height += border_top.border_width();
            content_height += border_bottom.border_width();

            auto actual_height = content_height + padding.vertical() + top_margin;

            resolve_style_height_releated_props(style, actual_height, true);

            param.height = actual_height;
            return param;
        }

        auto compute_layout(
            xml::Context const* context,
            core::BoundingBox container,
            node_index_t node = 0,
            int level = 0
        ) -> void {
            auto& el = nodes[node];
            {
                auto const& style = context->styles[el.style_index];
                el.container = {
                    container.x,
                    container.y,
                    style.width.as_cell(),
                    style.height.as_cell()
                };
            }

            el.viewport.x = std::max(el.container.min_x(), container.min_x());
            el.viewport.width = std::min(el.container.max_x(), container.max_x()) - el.container.min_x();

            el.viewport.y = std::max(el.container.min_y(), container.min_y());
            el.viewport.height = std::min(el.container.max_y(), container.max_y()) - el.container.min_y();

            auto tmp_vp = el.container;

            auto v_margin = std::make_pair(0 /*top*/, 0 /*bottom*/);

            auto previous_node_start = node_index_t{};

            auto top_margin = 0;
            auto previous_end_margin = 0;

            for (auto j = 0ul; j < el.children.size(); ++j) {
                auto c = el.children[j];
                auto& ch = nodes[c];
                auto const& style = context->styles[ch.style_index];

                v_margin = {
                    std::max(v_margin.first, style.margin.top.as_cell()),
                    std::max(v_margin.second, style.margin.bottom.as_cell())
                };

                auto vp = tmp_vp;
                vp.width = std::min(style.width.as_cell(), tmp_vp.width);
                vp.height = std::min(style.height.as_cell(), tmp_vp.height);

                ch.content_offset_x = style.padding.left.as_cell() + std::abs(previous_end_margin - style.margin.left.as_cell()) + style.border_left.border_width();
                ch.content_offset_y = style.padding.top.as_cell() + style.border_top.border_width();
                previous_end_margin = style.margin.right.as_cell();

                ch.content_offset_x += std::max(el.content_offset_x - el.container.x, 0);
                ch.content_offset_y += std::max(el.content_offset_y - el.container.y, 0);

                compute_layout(context, vp, c, level + 1);

                if (ch.start_from_newline) {
                    tmp_vp.x = el.container.min_x();
                    tmp_vp.y += vp.height;

                    auto margin = std::abs(v_margin.first - top_margin);
                    for (auto i = previous_node_start; i < c; ++i) {
                        auto& tmp = nodes[i];
                        tmp.content_offset_y += margin;
                    }
                    previous_node_start = c;
                    top_margin = v_margin.second;
                    v_margin = {};
                } else {
                    tmp_vp.x = vp.max_x();
                }
            }
        }

        template <core::detail::IsScreen S>
        auto render_node(
            core::Device<S>& dev,
            xml::Context const* context,
            node_index_t node,
            bool ignore_scroll = false,
            bool is_next_element_inline = false
        ) -> void {
            auto& el = nodes[node];
            auto const& style = context->styles[el.style_index];

            if (el.tag.empty() && node != 0) {
                text::TextRenderer t {
                    .text = el.trim_end_whitespace == false ? el.text : core::utils::rtrim(el.text),
                    .container = el.container,
                    .start_offset = {
                        el.content_offset_x + el.container.x,
                        el.content_offset_y + el.container.y
                    },
                };
                t.render(dev, style);
                return;
            }

            if (!ignore_scroll && (el.is_scrollable_x() || el.is_scrollable_y())) {
                auto d = core::Device(&el.canvas);
                render_node(d, context, node, true);
                core::ViewportClipGuard clip(dev, el.viewport);
                for (auto r = 0; r < el.canvas.rows(); ++r) {
                    for (auto c = 0; c < el.canvas.cols(); ++c) {
                        auto const& cell = el.canvas(static_cast<unsigned>(r), static_cast<unsigned>(c));
                        auto x = c + el.container.x;
                        auto y = r + el.container.y;
                        dev.put_pixel(cell.text(), x, y, cell.style);
                    }
                }
            } else {
                for (auto i = 0ul; i < el.children.size(); ++i) {
                    auto c = el.children[i];
                    auto const& ch = nodes[c];

                    if (ch.tag.empty()) {
                        // std::println("HERE: \t\t\t\t\t\t\t\t{} | '{}' | {}, {}", ch.viewport, ch.text, ch.content_offset_x, ch.content_offset_y);
                        render_node(dev, context, c, false, is_next_element_inline);
                    } else {
                        // std::println("{:{}}HERE[{}]: {} | {}", ' ', 60, el.tag, el.text, ch.viewport);
                        core::ViewportClipGuard g(dev, ch.viewport);
                        render_node(dev, context, c, false, is_next_element_inline);
                    }
                }
            }

            auto [tl_border_style, tr_border_style, br_border_style, bl_border_style] = style.border_type;
            auto border_style = core::PixelStyle::from_style(style);
            if (style.border_top.width.as_cell() != 0) {
                // top
                auto set = style.border_top.char_set(tl_border_style);

                border_style.fg_color = style.border_top.color;
                for (auto c = el.container.min_x(); c < el.container.max_x(); ++c) {
                    auto r = el.container.min_y();
                    dev.put_pixel(set.horizonal, c, r, border_style);
                }
            }

            if (style.border_bottom.width.as_cell() != 0) {
                // bottom
                auto set = style.border_bottom.char_set(tl_border_style);

                border_style.fg_color = style.border_bottom.color;
                for (auto c = el.container.min_x(); c < el.container.max_x(); ++c) {
                    auto r = el.container.max_y() - 1;
                    dev.put_pixel(set.horizonal, c, r, border_style);
                }
            }

            if (style.border_left.width.as_cell() != 0) {
                // left
                auto set = style.border_left.char_set(tl_border_style);

                border_style.fg_color = style.border_left.color;
                for (auto r = el.container.min_y(); r < el.container.max_y(); ++r) {
                    auto c = el.container.min_x();
                    dev.put_pixel(set.vertical, c, r, border_style);
                }
            }

            if (style.border_left.width.as_cell() != 0) {
                // right
                auto set = style.border_right.char_set(tl_border_style);

                border_style.fg_color = style.border_right.color;
                for (auto r = el.container.min_y(); r < el.container.max_y(); ++r) {
                    auto c = el.container.max_x() - 1;
                    dev.put_pixel(set.vertical, c, r, border_style);
                }
            }
            { // corners
                std::string_view corner{};
                // top-left
                if (style.border_top.width.as_cell() != 0 && style.border_left.width.as_cell() != 0) {
                    corner = style.border_top.char_set(tl_border_style).top_left;
                }

                if (!corner.empty()) {
                    border_style.fg_color = style.border_left.color;
                    dev.put_pixel(corner, el.container.min_x(), el.container.min_y(), border_style);
                }
                corner = {};

                // top-right
                if (style.border_top.width.as_cell() != 0 && style.border_right.width.as_cell() != 0) {
                    corner = style.border_top.char_set(tr_border_style).top_right;
                }

                if (!corner.empty()) {
                    border_style.fg_color = style.border_right.color;
                    dev.put_pixel(corner, el.container.max_x() - 1, el.container.min_y(), border_style);
                }
                corner = {};

                // bottom-right
                if (style.border_bottom.width.as_cell() != 0 && style.border_right.width.as_cell() != 0) {
                    corner = style.border_bottom.char_set(br_border_style).bottom_right;
                }

                if (!corner.empty()) {
                    border_style.fg_color = style.border_right.color;
                    dev.put_pixel(corner, el.container.max_x() - 1, el.container.max_y() - 1, border_style);
                }
                corner = {};

                // bottom-right
                if (style.border_bottom.width.as_cell() != 0 && style.border_left.width.as_cell() != 0) {
                    corner = style.border_bottom.char_set(br_border_style).bottom_left;
                }

                if (!corner.empty()) {
                    border_style.fg_color = style.border_left.color;
                    dev.put_pixel(corner, el.container.min_x(), el.container.max_y() - 1, border_style);
                }
            }
        }
    };

} // namespace termml::layout

#endif // AMT_TERMML_LAYOUT_HPP
