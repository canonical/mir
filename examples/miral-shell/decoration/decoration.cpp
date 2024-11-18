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

#include "mir/geometry/forward.h"
#include "mir/geometry/rectangle.h"

#include "decoration.h"

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

void UserDecoration::render_titlebar(miral::Buffer titlebar_pixels, mir::geometry::Size scaled_titlebar_size)
{
    for (geometry::Y y{0}; y < as_y(scaled_titlebar_size.height); y += geometry::DeltaY{1})
    {
        miral::Renderer::render_row(
            titlebar_pixels, scaled_titlebar_size, {0, y}, scaled_titlebar_size.width, 0xFF00FFFF);
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

void UserDecoration::InputManager::process_up()
{
    if (active_widget)
    {
        widget_up(*active_widget.value());
    }
}

void UserDecoration::InputManager::widget_down(Widget& widget)
{
    widget.state = ButtonState::Down;
}

void UserDecoration::InputManager::widget_up(Widget& widget)
{
    widget.state = ButtonState::Hovered;
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

void UserDecoration::InputManager::update_window_state(WindowState const& window_state)
{
    for (auto const& widget : widgets)
    {
        widget->rect = resize_edge_rect(window_state, widget->resize_edge.value());
    }
}

auto UserDecoration::InputManager::resize_edge_rect(
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
        rect.size.height = window_state.bottom_border_height();
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


auto UserDecoration::create_manager(mir::Server& server)
    -> std::shared_ptr<miral::DecorationManagerAdapter>
{
    return miral::DecorationBasicManager(
               server,
               [](auto, auto)
               {
                   auto decoration = std::make_shared<UserDecoration>();
                   auto decoration_adapter = std::make_shared<miral::DecorationAdapter>(
                       decoration->redraw_notifier(),
                       [decoration](miral::Buffer titlebar_pixels, geometry::Size scaled_titlebar_size)
                       {
                           decoration->render_titlebar(titlebar_pixels, scaled_titlebar_size);
                       },
                       [](auto...)
                       {
                       },
                       [](auto...)
                       {
                       },
                       [](auto...)
                       {
                       },
                       [decoration](auto... args)
                       {
                           // Author should figure out if changes require a redraw
                           // then call DecorationRedrawNotifier::notify()
                           // Here we just assume we're always redrawing
                           /* mir::log_debug("process_enter"); */
                           decoration->input_manager.process_enter(args...);
                           decoration->redraw_notifier()->notify();
                       },
                       [decoration](auto... args)
                       {
                           /* mir::log_debug("process_leave"); */
                           decoration->input_manager.process_leave(args...);
                           decoration->redraw_notifier()->notify();
                       },
                       [decoration](auto...)
                       {
                           /* mir::log_debug("process_down"); */
                           decoration->input_manager.process_down();
                           decoration->redraw_notifier()->notify();
                       },
                       [decoration](auto...)
                       {
                           /* mir::log_debug("process_up"); */
                           decoration->input_manager.process_up();
                           decoration->redraw_notifier()->notify();
                       },
                       [decoration](auto...)
                       {
                           /* mir::log_debug("process_move"); */
                           decoration->redraw_notifier()->notify();
                       },
                       [decoration](auto... args)
                       {
                           /* mir::log_debug("process_drag"); */
                           decoration->input_manager.process_drag(args...);
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
                       [decoration](auto...)
                       {
                           decoration->redraw_notifier()->notify();
                       },
                       [decoration](auto window_state)
                       {
                           decoration->input_manager.update_window_state(*window_state);
                       });

                   return decoration_adapter;
               })
        .to_adapter();
}

