/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mir/geometry/dimensions.h"
#include "mir/geometry/forward.h"
#include "mir/geometry/rectangle.h"

#include "decoration.h"

#include "mir_toolkit/common.h"
#include "mir_toolkit/cursors.h"
#include "miral/decoration_adapter.h"
#include "miral/decoration.h"
#include "miral/decoration_window_state.h"

#include "miral/decoration_basic_manager.h"
#include "miral/decoration_manager_builder.h"

#include <memory>

namespace geometry = mir::geometry;
namespace geom = mir::geometry;

using miral::InputContext;

UserDecoration::UserDecoration() = default;

auto UserDecoration::InputManager::buttons() const -> std::vector<std::shared_ptr<Widget const>>
{
    std::vector<std::shared_ptr<Widget const>> button_rects;
    for (auto const& widget : widgets)
    {
        if (widget->button)
            button_rects.push_back(widget);
    }

    return button_rects;
}

void UserDecoration::render_titlebar(miral::Renderer::Buffer const& titlebar_buffer)
{
    fill_solid_color(titlebar_buffer, current_titlebar_color);

    auto const draw_button =
        [&]( geometry::Rectangle button_rect, miral::Pixel color)
    {
        auto start = button_rect.top_left;
        auto end = button_rect.bottom_right();
        for (auto y = start.y; y < end.y; y += geometry::DeltaY{1})
            miral::Renderer::render_row(titlebar_buffer, {start.x, y}, geometry::as_width(end.x - start.x), color);
    };

    auto buttons = input_manager.buttons();
    for (auto const& button : buttons)
    {
        auto button_rect = button->rect;
        auto color = 0xFFBB4444;
        switch(button->button.value())
        {
        case InputManager::ButtonFunction::Close:
            color = 0xFFBB4444; // Red
            break;
        case InputManager::ButtonFunction::Maximize:
            color = 0xFF44BB44; // Green
            break;
        case InputManager::ButtonFunction::Minimize:
            color = 0xFFBB8833; // Yellow
            break;
        }

        auto const should_highlight =
            input_manager.active_widget.has_value() && input_manager.active_widget.value()->rect == button_rect;

        if (should_highlight)
            color += 0x00444444;

        draw_button(button_rect, color);
    }
}

void UserDecoration::fill_solid_color(miral::Renderer::Buffer const& left_border_buffer, miral::Pixel color)
{
    for (geometry::Y y{0}; y < as_y(left_border_buffer.size().height); y += geometry::DeltaY{1})
    {
        miral::Renderer::render_row(left_border_buffer, {0, y}, left_border_buffer.size().width, color);
    }
}

auto UserDecoration::InputManager::widget_at(geom::Point location) -> std::optional<std::shared_ptr<Widget>>
{
    for (auto const& widget : widgets)
    {
        if (widget->rect.contains(location))
            return widget;
    }
    return std::nullopt;
}

void UserDecoration::InputManager::process_drag(miral::DeviceEvent& device, InputContext ctx)
{
    auto const new_widget = widget_at(device.location());

    if (active_widget)
    {
        widget_drag(*active_widget.value(), ctx);
        if (new_widget != active_widget)
            widget_leave(*active_widget.value(), ctx);
    }

    bool const enter = new_widget && (active_widget != new_widget);
    active_widget = new_widget;
    if (enter)
        widget_enter(*active_widget.value(), ctx);
}

void UserDecoration::InputManager::process_enter(miral::DeviceEvent& device, InputContext ctx)
{
    active_widget = widget_at(device.location());
    if (active_widget)
        widget_enter(*active_widget.value(), ctx);
}

void UserDecoration::InputManager::process_leave(InputContext ctx)
{
    if (active_widget)
    {
        widget_leave(*active_widget.value(), ctx);
        active_widget = std::nullopt;
    }
}

void UserDecoration::InputManager::process_down()
{
    if (active_widget)
    {
        widget_down(*active_widget.value());
    }
}

void UserDecoration::InputManager::process_up(InputContext ctx)
{
    if (active_widget)
    {
        widget_up(*active_widget.value(), ctx);
    }
}

void UserDecoration::InputManager::process_move(miral::DeviceEvent& device, InputContext ctx)
{
    auto const new_widget = widget_at(device.location());
    if (new_widget != active_widget)
    {
        if (active_widget)
            widget_leave(*active_widget.value(), ctx);

        active_widget = new_widget;

        if (active_widget)
            widget_enter(*active_widget.value(), ctx);
    }
}

void UserDecoration::InputManager::widget_down(Widget& widget)
{
    widget.state = ButtonState::Down;
}

void UserDecoration::InputManager::widget_up(Widget& widget, InputContext ctx)
{
    widget.state = ButtonState::Hovered;
    if (widget.button)
    {
        switch (widget.button.value())
        {
        case ButtonFunction::Close:
            ctx.request_close();
            break;

        case ButtonFunction::Maximize:
            ctx.request_toggle_maximize();
            break;

        case ButtonFunction::Minimize:
            // For some reason, Mir forces the app window to be visible on click
            ctx.request_minimize();
            break;
        }
    }
}

void UserDecoration::InputManager::widget_drag(Widget& widget, miral::InputContext ctx)
{
    if (widget.state == ButtonState::Down)
    {
        if (widget.resize_edge)
        {
            auto edge = widget.resize_edge.value();

            if (edge == mir_resize_edge_none)
            {
                ctx.request_move();
            }
            else
            {
                ctx.request_resize(edge);
            }
        }
        widget.state = ButtonState::Up;
    }
    widget.state = ButtonState::Hovered;
}

auto resize_edge_to_cursor_name(MirResizeEdge resize_edge) -> std::string
{
    std::string cursor_name;
    switch (resize_edge)
    {
    case mir_resize_edge_north:
    case mir_resize_edge_south:
        cursor_name = mir_vertical_resize_cursor_name;
        break;

    case mir_resize_edge_east:
    case mir_resize_edge_west:
        cursor_name = mir_horizontal_resize_cursor_name;
        break;

    case mir_resize_edge_northeast:
    case mir_resize_edge_southwest:
        cursor_name = mir_diagonal_resize_bottom_to_top_cursor_name;
        break;

    case mir_resize_edge_northwest:
    case mir_resize_edge_southeast:
        cursor_name = mir_diagonal_resize_top_to_bottom_cursor_name;
        break;

    default:
        cursor_name = mir_default_cursor_name;
    }

    return cursor_name;
}

void UserDecoration::InputManager::widget_leave(Widget& widget, miral::InputContext ctx) {
    widget.state = ButtonState::Up;
    ctx.set_cursor(resize_edge_to_cursor_name(mir_resize_edge_none));
}

void UserDecoration::InputManager::widget_enter(Widget& widget, miral::InputContext ctx)
{
    widget.state = ButtonState::Hovered;
    if(widget.resize_edge)
        ctx.set_cursor(resize_edge_to_cursor_name(widget.resize_edge.value()));
}

auto button_rect(WindowState const& window_state, unsigned n) -> geom::Rectangle
{
    geom::Rectangle titlebar = window_state.titlebar_rect();
    auto button_width = window_state.geometry().button_width;
    auto padding_between_buttons = window_state.geometry().padding_between_buttons;

    geom::X x = titlebar.right() - as_delta(window_state.side_border_width()) -
                n * as_delta(button_width + padding_between_buttons) - as_delta(button_width);

    return geom::Rectangle{{x, titlebar.top()}, {button_width, titlebar.size.height}};
}

auto resize_edge_rect(
    WindowState const& window_state,
    MirResizeEdge resize_edge) -> geom::Rectangle
{
    switch (resize_edge)
    {
    case mir_resize_edge_none:
        return window_state.titlebar_rect();

    case mir_resize_edge_north:
    {
        auto rect = window_state.titlebar_rect();

        // Cap the resize edge to half of the titlebar height
        auto bottom_height = window_state.bottom_border_height();
        auto top_height = window_state.titlebar_height();
        auto const min_titlebar_drag_height = top_height / 2;

        rect.size.height = std::min(min_titlebar_drag_height, bottom_height);
        return rect;
    }

    case mir_resize_edge_south:
        return window_state.bottom_border_rect();

    case mir_resize_edge_west:
        return window_state.left_border_rect();

    case mir_resize_edge_east:
        return window_state.right_border_rect();

    case mir_resize_edge_northwest:
        return {{}, window_state.resize_corner_input_size()};

    case mir_resize_edge_northeast:
    {
        geom::Point top_left{
            as_x(window_state.window_size().width) -
                as_delta(window_state.resize_corner_input_size().width),
            0};
        return {top_left, window_state.resize_corner_input_size()};
    }

    case mir_resize_edge_southwest:
    {
        geom::Point top_left{
            0,
            as_y(window_state.window_size().height) -
                as_delta(window_state.resize_corner_input_size().height)};
        return {top_left, window_state.resize_corner_input_size()};
    }

    case mir_resize_edge_southeast:
    {
        geom::Point top_left{
            as_x(window_state.window_size().width) -
                as_delta(window_state.resize_corner_input_size().width),
            as_y(window_state.window_size().height) -
                as_delta(window_state.resize_corner_input_size().height)};
        return {top_left, window_state.resize_corner_input_size()};
    }


    // TODO: the rest
    default: return {};
    }
}

void UserDecoration::InputManager::update_window_state(WindowState const& window_state)
{
    auto button_index = 0;
    for (auto const& widget : widgets)
    {
        if (widget->resize_edge)
            widget->rect = resize_edge_rect(window_state, widget->resize_edge.value());
        else
        {
            widget->rect = button_rect(window_state, button_index);
            button_index += 1;
        }
    }
}

void UserDecoration::unfocused()
{
    current_titlebar_color = unfocused_titlebar_color;
}

void UserDecoration::focused()
{
    current_titlebar_color = focused_titlebar_color;
}

auto UserDecoration::create_manager(mir::Server& server)
    -> std::shared_ptr<miral::DecorationManagerAdapter>
{
    using namespace mir::geometry;
    // Shared between all decorations.
    auto const custom_geometry = std::make_shared<StaticGeometry>(StaticGeometry{
        .titlebar_height = Height{24},
        .side_border_width = geom::Width{6},
        .bottom_border_height = geom::Height{6},
        .resize_corner_input_size = geom::Size{16, 16},
        .button_width = geom::Width{24},
        .padding_between_buttons = geom::Width{6},
    });

    return miral::DecorationBasicManager(
               server,
               [custom_geometry](auto, auto)
               {

                   auto decoration = std::make_shared<UserDecoration>();
                   auto decoration_adapter = std::make_shared<miral::DecorationAdapter>(
                       decoration->redraw_notifier(),
                       [decoration](auto const& titlebar_buffer)
                       {
                           decoration->render_titlebar(titlebar_buffer);
                       },
                       [decoration](auto const& left_border_buffer)
                       {
                            decoration->fill_solid_color(left_border_buffer, decoration->current_titlebar_color);
                       },
                       [decoration](auto const& right_border_buffer)
                       {
                            decoration->fill_solid_color(right_border_buffer, decoration->current_titlebar_color);
                       },
                       [decoration](auto const& bottom_border_buffer)
                       {
                            decoration->fill_solid_color(bottom_border_buffer, decoration->current_titlebar_color);
                       },
                       [decoration](auto... args)
                       {
                           // Author should figure out if changes require a redraw
                           // then call DecorationRedrawNotifier::notify()
                           // Here we just assume we're always redrawing
                           decoration->input_manager.process_enter(args...);
                           decoration->redraw_notifier()->notify();
                       },
                       [decoration](auto... args)
                       {
                           decoration->input_manager.process_leave(args...);
                           decoration->redraw_notifier()->notify();
                       },
                       [decoration](auto...)
                       {
                           decoration->input_manager.process_down();
                       },
                       [decoration](auto... args)
                       {
                           decoration->input_manager.process_up(args...);
                       },
                       [decoration](auto... args)
                       {
                           decoration->input_manager.process_move(args...);
                           decoration->redraw_notifier()->notify();
                       },
                       [decoration](auto... args)
                       {
                           decoration->input_manager.process_drag(args...);
                       },
                       [decoration](auto*, MirWindowAttrib attrib, auto value)
                       {
                           switch (attrib)
                           {
                           case mir_window_attrib_focus:
                               switch (value)
                               {
                               case mir_window_focus_state_unfocused:
                                   decoration->unfocused();
                                   break;

                               default:
                                   decoration->focused();
                                   break;
                               }
                               break;
                           default:
                               break;
                           }
                           decoration->redraw_notifier()->notify();
                       },
                       [decoration](auto...)
                       {
                           decoration->redraw_notifier()->notify();
                       },
                       [decoration](auto...)
                       {
                           decoration->redraw_notifier()->notify();
                       },
                       [decoration](auto window_state)
                       {
                           decoration->input_manager.update_window_state(*window_state);
                       });

                   decoration_adapter->set_custom_geometry(custom_geometry);

                   return decoration_adapter;
               })
        .to_adapter();
}
