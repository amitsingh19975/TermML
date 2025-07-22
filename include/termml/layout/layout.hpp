#ifndef AMT_TERMML_LAYOUT_HPP
#define AMT_TERMML_LAYOUT_HPP

#include "../css/style.hpp"
#include "../core/bounding_box.hpp"
#include "../core/point.hpp"
#include "../core/terminal.hpp"
#include "../core/device.hpp"
#include "../core/string_utils.hpp"
#include "../css/utils.hpp"
#include "text.hpp"
#include "line_box.hpp"
#include "../xml/node.hpp"
#include <__ostream/print.h>
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
        LineSpan lines{};
        std::vector<node_index_t> children{};
        core::BoundingBox container{};

        bool scrollable_x{false};
        bool scrollable_y{false};

        core::Terminal canvas{};
    };

    struct LayoutContext {
        core::BoundingBox viewport;
        std::vector<LayoutNode> nodes;
        std::vector<LineBox> lines;

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
            lines.clear();
            auto layout = LayoutNode {
                .tag = {},
                .node_index = 0,
                .style_index = 0
            };

            nodes.push_back(std::move(layout));
            initialize_nodes(context, { .index = 0, .kind = xml::NodeKind::Element }, 0);
            resolve_style(context);
            resolve_cyclic_width(context, 0, viewport.width);
            resolve_cyclic_height(context, 0, {
                .height = viewport.height,
                .content = viewport,
                .start_position = { viewport.min_x(), viewport.min_y() }
            });
            // compute_layout(context, viewport);
        }

        template <core::detail::IsScreen S>
        auto render(core::Device<S>& dev, xml::Context const* context, node_index_t node = 0) -> void {
            render_node(dev, context, node, viewport);
        }

        auto dump(xml::Context const* context, node_index_t index = 0, unsigned level = 0) const -> void {
            auto tab = level * 4;
            auto const& l = nodes[index];
            std::println("{:{}} > {}", ' ', tab, l.tag);
            if(l.tag.empty() && index != 0) {
                std::println("{:{}}   |- Text: '{}'", ' ', tab, l.text);
            }

            std::println("{:{}}   |- Container: {}", ' ', tab, l.container);
            std::println("{:{}}   |- Lines: {}", ' ', tab, std::span(lines.data() + l.lines.start, l.lines.size));
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
            auto& layout = nodes[node];
            auto& style = context->styles[layout.style_index];

            if (node == 0) {
                style.width = css::Number {
                    .i = static_cast<int>(viewport.width),
                    .unit = css::Unit::Cell
                };

                style.height = css::Number {
                    .i = static_cast<int>(viewport.height),
                    .unit = css::Unit::Cell
                };

                style.inset = {
                    .top = css::Number::from_cell(static_cast<int>(viewport.min_y())),
                    .right = css::Number::from_cell(static_cast<int>(viewport.max_x())),
                    .bottom = css::Number::from_cell(static_cast<int>(viewport.max_y())),
                    .left = css::Number::from_cell(static_cast<int>(viewport.min_x())),
                };
            }

            for (auto l: layout.children) {
                auto& ts = context->styles[nodes[l].style_index];
                if (style.width.is_absolute()) {
                    resolve_style_width_releated_props(ts, style.width.i);
                }
                ts.margin = ts.margin.resolve(style.width.i);
                resolve_style(context, l);
            }

            for (auto l: layout.children) {
                auto& ts = context->styles[nodes[l].style_index];
                if (style.height.is_absolute()) {
                    resolve_style_height_releated_props(ts, style.height.i);
                }
                resolve_style(context, l);
            }
        }

        static constexpr auto resolve_style_width_releated_props(
            css::Style& style,
            int parent_width,
            bool resolve_auto_fit = false
        ) noexcept -> void {
            parent_width = std::max(parent_width, 0);

            if (resolve_auto_fit) {
                style.width = style.width.resolve_all(parent_width);
                style.min_width = style.min_width.resolve_all(parent_width);
                style.max_width = style.max_width.resolve_all(parent_width);
                style.width.i = std::max(style.width.i, style.min_width.i);
                if (style.overflow_x == css::Overflow::Clip) {
                    style.width.i = std::min({ style.width.i, style.max_width.i, parent_width });
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
            int max_parent_width
        ) noexcept -> int /*container width*/ {
            auto& el = nodes[node];
            auto& style = context->styles[el.style_index];
            auto content_width = 0;

            if (el.tag.empty() && node != 0) {
                auto text = TextLayouter{
                    .text = el.text,
                };
                content_width = text.measure_width();
                if (style.whitespace != css::Whitespace::NoWrap) {
                    content_width = std::min(content_width, max_parent_width);
                }
            }

            auto last_inline_element = false;
            for (auto i = 0ul; i < el.children.size(); ++i) {
                auto l = el.children[i];
                auto const& c = nodes[l];
                auto& cs = context->styles[c.style_index];
                cs.margin = cs.margin.resolve(max_parent_width);
                auto is_inline = cs.is_inline_context();
                auto margin = cs.margin.horizontal();

                if (cs.width.is_absolute()) {
                    auto w = cs.width.i; 
                    if (is_inline == last_inline_element && is_inline == true) {
                        w += content_width;
                    }
                    content_width = std::max(content_width, w);
                    resolve_cyclic_width(context, l, cs.width.i);
                } else if (cs.width.is_fit()) {
                    auto parent_width = style.width.is_absolute() ? style.width.i : max_parent_width;
                    if (is_inline == last_inline_element && is_inline == true) {
                        parent_width += content_width;
                    }
                    content_width = std::max(
                        content_width,
                        resolve_cyclic_width(context, l, parent_width)
                    );
                } else if (cs.width.is_precentage()) {
                    resolve_style_width_releated_props(cs, max_parent_width, true);
                    content_width = std::max(content_width, cs.width.i);
                }

                last_inline_element = is_inline;
                content_width += margin;
            }

            if (style.width.is_absolute()) {
                el.container.width = style.width.i;
                return el.container.width;
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

            el.container.width = actual_width;
            resolve_style_width_releated_props(style, el.container.width, true);

            return actual_width;
        }

        static constexpr auto resolve_style_height_releated_props(
            css::Style& style,
            int parent_height,
            bool resolve_auto_fit = false
        ) noexcept -> void {
            if (resolve_auto_fit) {
                style.min_height = style.min_height.resolve_all(parent_height);
                style.max_height = style.max_height.resolve_all(parent_height);
                style.height = style.height.resolve_all(parent_height);
                style.height.i = std::max(style.height.i, style.min_height.i);
                if (style.overflow_y == css::Overflow::Clip) {
                    style.height.i = std::min({ style.height.i, style.max_height.i, parent_height });
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
            if (s.has_start_whitespace()) return { 0, true };
            if (!s.has_inline_flow()) return { 0, true };

            if (s.display == css::Display::InlineBlock) return {
                s.width.i,
                true
            };

            if (el.tag.empty()) {
                auto pos = detail::find_word(el.text);
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
            core::BoundingBox content{};
            core::Point start_position{};
            std::size_t previous_line{};
        };

        constexpr auto resolve_cyclic_height(
            xml::Context* context,
            node_index_t node,
            HeightResult param
        ) -> HeightResult {
            auto& el = nodes[node];
            auto& p_style = context->styles[el.style_index];
            if (!el.text.empty()) {
                auto t = TextLayouter {
                    .text = el.text,
                    .container = {
                        .x = param.content.x,
                        .y = param.content.y,
                        .width = param.content.width,
                        .height = core::BoundingBox::inf().height - param.content.y // avoid overflow
                    },
                    .start_position = param.start_position,
                };

                auto result = t(lines, param.previous_line, p_style);
                el.lines = result.span;
                return {
                    .height = result.container.height,
                    .content = param.content,
                    .start_position = t.start_position
                };
            }

            auto v_margin = std::make_pair(0 /*top*/, 0 /*bottom*/);
            auto v_padding = std::make_pair(0 /*top*/, 0 /*bottom*/);
            auto is_previous_inline = false;

            param.height = 0;
            auto tmp_param = param;

            auto line_start = std::max<std::size_t>(lines.size(), 1) - 1;
            auto margin_line_start = line_start;
            auto margin_node_start = std::size_t{};
            for (auto i = std::size_t{}; i < el.children.size(); ++i) {
                auto l = el.children[i];
                auto& ch = nodes[l];
                auto& style = context->styles[ch.style_index];

                auto top_margin = style.margin.top.as_cell();
                auto bottom_margin = style.margin.bottom.as_cell();
                auto top_padding = style.padding.top.as_cell();
                auto bottom_padding = style.padding.bottom.as_cell();

                if (style.display == css::Display::Inline) {
                    top_margin = 0;
                    bottom_margin = 0;
                    top_padding = 0;
                    bottom_padding = 0;
                }

                if (style.can_collapse_margin()) {
                    if (v_margin.first < 0 && top_margin < 0) {
                        v_margin.first = std::min(v_margin.first, top_margin);
                    } else if (v_margin.first > 0 && top_margin > 0) {
                        v_margin.first = std::max(v_margin.first, top_margin);
                    } else {
                        v_margin.first += top_margin;
                    }

                    if (v_margin.second < 0 && bottom_margin < 0) {
                        v_margin.second = std::min(v_margin.second, bottom_margin);
                    } else if (v_margin.second > 0 && bottom_margin > 0) {
                        v_margin.second = std::max(v_margin.second, bottom_margin);
                    } else {
                        v_margin.second += bottom_margin;
                    }

                } else {
                    v_margin = {
                        v_margin.first + style.margin.top.as_cell(),
                        v_margin.second + style.margin.bottom.as_cell(),
                    };
                }
                v_padding = {
                    std::max(v_padding.first, top_padding),
                    std::max(v_padding.second, bottom_padding),
                };

                auto tmp = tmp_param;
                auto offset_x = style.padding.left.as_cell() + style.border_left.border_width() + style.margin.left.as_cell();
                auto offset_y = top_padding + style.border_top.border_width();
                tmp.start_position = {
                    .x = tmp_param.start_position.x + offset_x,
                    .y = tmp_param.start_position.y + offset_y
                };

                auto is_inline = style.has_inline_flow();

                if (!is_inline) {
                    tmp.content.x = tmp.start_position.x;
                    tmp.content.y = tmp_param.start_position.y + tmp_param.height;
                    auto x_shift = tmp.content.x - ch.container.x;
                    ch.container.x = tmp.content.x;
                    ch.container.y = tmp.content.y;
                    ch.container.width -= x_shift;
                    tmp.content.x += offset_x;
                    tmp.content.y += offset_y;

                    auto width = std::max(tmp_param.content.width - (style.padding.horizontal() + style.border_right.border_width() + style.border_left.border_width() + style.margin.horizontal()), 0);

                    tmp.content = core::BoundingBox::from(
                        tmp.content.x,
                        tmp_param.content.x + width,
                        tmp.content.y,
                        core::BoundingBox::inf().y - tmp.content.y
                    );

                    tmp.height = tmp_param.height = 0;
                    tmp.start_position = { tmp.content.x, tmp.content.y };
                } else {
                    ch.container.x = tmp.content.x;
                    ch.container.y = tmp.content.y;
                }

                tmp.content.width = std::min(tmp.content.width, ch.container.width);

                // if (style.whitespace != css::Whitespace::NoWrap) {
                //     ch.container.width = tmp.content.width + offset_x;
                // }

                if (is_previous_inline && style.is_inline_context()) {
                    tmp.previous_line = std::max(lines.size(), std::size_t{1}) - 1;
                }

                tmp = resolve_cyclic_height(context, l, tmp);
                if (style.height.is_fit()) {
                    ch.container.height = tmp.height;
                } else if (style.height.is_absolute()) {
                    ch.container.height = style.height.i;
                } else {
                    ch.container.height = 0;
                }
                ch.container.height += style.padding.vertical() + style.border_bottom.border_width() + style.border_top.border_width();

                auto moved_to_new_line = (tmp.start_position.y != tmp_param.start_position.y);

                auto height = tmp_param.height;
                tmp_param = tmp;

                if (moved_to_new_line || !is_inline) {
                    height += ch.container.height;

                    for (auto j = margin_line_start; j < lines.size(); ++j) {
                        lines[j].bounds.y += v_margin.first;
                    }
                    for (auto j = margin_node_start; j < i; ++j) {
                        nodes[el.children[j]].container.y += v_margin.first;
                    }

                    v_margin = { v_margin.second, 0 };
                    v_padding = {};
                    height += v_margin.first;
                    margin_line_start = lines.size();
                    margin_node_start = i + 1;
                }
                if (is_previous_inline && is_inline) {
                    height = std::max(height - 1, 0);
                }

                if (!moved_to_new_line && !is_inline) {
                    height = std::max(height - 1, 0);
                }

                tmp_param.height = height;

                if (!is_inline) {
                    param.height += tmp_param.height;
                    param.start_position.y += tmp_param.height;
                    param.start_position.x = param.content.x;

                    tmp_param = param;
                    tmp_param.height = 0;
                } else {
                    // tmp_param.start_offset.x += 1;
                }

                is_previous_inline = is_inline;
                resolve_style_height_releated_props(style, ch.container.height, true);
            }

            for (auto j = margin_line_start; j < lines.size(); ++j) {
                lines[j].bounds.y += v_margin.first;
            }
            for (auto j = margin_node_start; j < el.children.size(); ++j) {
                nodes[el.children[j]].container.y += v_margin.first;
            }

            param.height += tmp_param.height;
            if (p_style.has_inline_flow()) {
                param.start_position.x = tmp_param.start_position.x;
                param.start_position.y = tmp_param.start_position.y;
            } else {
                param.start_position.x = param.content.x;
                param.start_position.y += param.height;
            }

            auto line_end = lines.size();
            el.lines = {
                .start = static_cast<unsigned>(line_start),
                .size = static_cast<unsigned>(line_end - line_start)
            };
            if (node == 0) {
                el.container = param.content;
            }
            return param;
        }

        // auto compute_layout(
        //     xml::Context const* context,
        //     core::BoundingBox container,
        //     node_index_t node = 0
        // ) -> void {
        //     auto& el = nodes[node];
        //     auto x = container.x;
        //     auto y = container.y;
        //     el.scrollable_x = (el.container.max_x() > container.max_x() || el.container.min_x() < container.min_x());
        //     el.scrollable_y = (el.container.max_y() > container.max_y() || el.container.min_y() < container.min_y());
        //     for (auto c: el.children) {
        //         auto& ch = nodes[c];
        //         auto const& style = context->styles[ch.style_index];
        //         ch.container.x = x;
        //         ch.container.y = y;
        //
        //         compute_layout(context, ch.container, c);
        //
        //         if (style.has_inline_flow()) {
        //             x += ch.container.width;
        //         } else {
        //             x = el.container.x;
        //             y += ch.container.height;
        //         }
        //     }
        //
        //     el.container.x = container.x;
        //     el.container.y = container.y;
        // }

        template <core::detail::IsScreen S>
        auto render_node(
            core::Device<S>& dev,
            xml::Context const* context,
            node_index_t node,
            core::BoundingBox container,
            bool ignore_scroll = false,
            bool is_next_element_inline = false
        ) -> void {
            auto& el = nodes[node];
            auto const& style = context->styles[el.style_index];

            if (el.tag.empty() && node != 0) {
                for (auto i = 0ul; i < el.lines.size; ++i) {
                    auto const& line = lines[el.lines.start + i];
                    // std::println("HERE: {} | {}", line.line, line.bounds);
                    dev.write_text(line.line, line.bounds.x, line.bounds.y, core::PixelStyle::from_style(style));
                }
                return;
            }

            if (!ignore_scroll && (el.scrollable_x || el.scrollable_y)) {
                auto d = core::Device(&el.canvas);
                render_node(d, context, node, el.container, true);
                core::ViewportClipGuard clip(dev, container);
                for (auto r = 0; r < el.canvas.rows(); ++r) {
                    for (auto c = 0; c < el.canvas.cols(); ++c) {
                        auto const& cell = el.canvas(static_cast<unsigned>(r), static_cast<unsigned>(c));
                        auto x = c + el.container.x;
                        auto y = r + el.container.y;
                        dev.put_pixel(cell.text(), x, y, cell.style);
                    }
                }
            } else {
                // core::ViewportClipGuard g(dev, container);
                for (auto i = 0ul; i < el.children.size(); ++i) {
                    auto c = el.children[i];
                    auto const& ch = nodes[c];

                    if (ch.tag.empty()) {
                        // std::println("HERE: \t\t\t\t\t\t\t\t{} | '{}' | {}, {}", ch.viewport, ch.text, ch.content_offset_x, ch.content_offset_y);
                        render_node(dev, context, c, container, false, is_next_element_inline);
                    } else {
                        render_node(dev, context, c, ch.container, false, is_next_element_inline);
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
