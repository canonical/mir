/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "input.h"
#include "mir/shell/decoration/input_resolver.h"
#include "window.h"
#include "threadsafe_access.h"
#include "basic_decoration.h"

#include "mir/scene/surface.h"
#include "mir_toolkit/cursors.h"

namespace ms = mir::scene;
namespace geom = mir::geometry;
namespace msd = mir::shell::decoration;

using namespace std::chrono_literals;

std::chrono::nanoseconds const msd::InputManager::double_click_threshold = 250ms;

msd::InputManager::InputManager(
    std::shared_ptr<StaticGeometry const> const& static_geometry,
    std::shared_ptr<ms::Surface> const& decoration_surface,
    WindowState const& window_state,
    std::shared_ptr<ThreadsafeAccess<BasicDecoration>> const& decoration)
    : InputResolver{decoration_surface},
      static_geometry{static_geometry},
      decoration{decoration},
      widgets{
          std::make_shared<Widget>(ButtonFunction::Close),
          std::make_shared<Widget>(ButtonFunction::Maximize),
          std::make_shared<Widget>(ButtonFunction::Minimize),
          std::make_shared<Widget>(mir_resize_edge_northwest),
          std::make_shared<Widget>(mir_resize_edge_northeast),
          std::make_shared<Widget>(mir_resize_edge_southwest),
          std::make_shared<Widget>(mir_resize_edge_southeast),
          std::make_shared<Widget>(mir_resize_edge_north),
          std::make_shared<Widget>(mir_resize_edge_south),
          std::make_shared<Widget>(mir_resize_edge_west),
          std::make_shared<Widget>(mir_resize_edge_east),
          std::make_shared<Widget>(mir_resize_edge_none)}
{
    set_cursor(mir_resize_edge_none);
    update_window_state(window_state);
}

msd::InputManager::~InputManager()
{
}

void msd::InputManager::update_window_state(WindowState const& window_state)
{
    std::lock_guard lock{mutex};

    unsigned button_index = 0;
    for (auto const& widget : widgets)
    {
        if (widget->button)
        {
            widget->rect = window_state.button_rect(button_index);
            button_index++;
        }
        else if (widget->resize_edge)
        {
            widget->rect = resize_edge_rect(window_state, *static_geometry, widget->resize_edge.value());
        }
        else
        {
            fatal_error("Invalid widget");
        }
    }

    switch (window_state.border_type())
    {
    case BorderType::Full:
        input_shape = {
            window_state.titlebar_rect(),
            window_state.left_border_rect(),
            window_state.right_border_rect(),
            window_state.bottom_border_rect(),
        };
        break;
    case BorderType::Titlebar:
        input_shape = {
            window_state.titlebar_rect(),
        };
        break;
    case BorderType::None:
        input_shape = {
            geom::Rectangle{} // Empty vector results in automatic input shape
        };
        break;
    }
}

auto msd::InputManager::state() -> std::unique_ptr<InputState>
{
    std::lock_guard lock{mutex};
    std::vector<ButtonInfo> buttons;
    for (auto const& widget : widgets)
    {
        if (widget->button)
            buttons.push_back(ButtonInfo{
                widget->button.value(),
                widget->state,
                widget->rect});
    }
    return std::make_unique<InputState>(
        std::move(buttons),
        input_shape);
}

auto msd::InputManager::resize_edge_rect(
    WindowState const& window_state,
    StaticGeometry const& static_geometry,
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
        return {{}, static_geometry.resize_corner_input_size};

    case mir_resize_edge_northeast:
    {
        geom::Point top_left{
            as_x(window_state.window_size().width) -
                as_delta(static_geometry.resize_corner_input_size.width),
            0};
        return {top_left, static_geometry.resize_corner_input_size};
    }

    case mir_resize_edge_southwest:
    {
        geom::Point top_left{
            0,
            as_y(window_state.window_size().height) -
                as_delta(static_geometry.resize_corner_input_size.height)};
        return {top_left, static_geometry.resize_corner_input_size};
    }

    case mir_resize_edge_southeast:
    {
        geom::Point top_left{
            as_x(window_state.window_size().width) -
                as_delta(static_geometry.resize_corner_input_size.width),
            as_y(window_state.window_size().height) -
                as_delta(static_geometry.resize_corner_input_size.height)};
        return {top_left, static_geometry.resize_corner_input_size};
    }

    default: return {};
    }
}


void msd::InputManager::process_enter(DeviceEvent& device)
{
    active_widget = widget_at(device.location);
    if (active_widget)
        widget_enter(*active_widget.value());
}

void msd::InputManager::process_leave()
{
    if (active_widget)
    {
        widget_leave(*active_widget.value());
        active_widget = std::nullopt;
    }
}

void msd::InputManager::process_down()
{
    if (active_widget)
    {
        widget_down(*active_widget.value());
    }
}

void msd::InputManager::process_up()
{
    if (active_widget)
    {
        widget_up(*active_widget.value());
    }
    auto const input_ev = mir_event_get_input_event(latest_event().get());
    previous_up_timestamp = std::chrono::nanoseconds{mir_input_event_get_event_time(input_ev)};
}

void msd::InputManager::process_move(DeviceEvent& device)
{
    auto const new_widget = widget_at(device.location);
    if (new_widget != active_widget)
    {
        if (active_widget)
            widget_leave(*active_widget.value());

        active_widget = new_widget;

        if (active_widget)
            widget_enter(*active_widget.value());
    }
}

void msd::InputManager::process_drag(DeviceEvent& device)
{
    auto const new_widget = widget_at(device.location);

    if (active_widget)
    {
        widget_drag(*active_widget.value());
        if (new_widget != active_widget)
            widget_leave(*active_widget.value());
    }

    bool const enter = new_widget && (active_widget != new_widget);
    active_widget = new_widget;
    if (enter)
        widget_enter(*active_widget.value());
}

auto msd::InputManager::widget_at(geom::Point location) -> std::optional<std::shared_ptr<Widget>>
{
    for (auto const& widget : widgets)
    {
        if (widget->rect.contains(location))
            return widget;
    }
    return std::nullopt;
}

void msd::InputManager::widget_enter(Widget& widget)
{
    widget.state = ButtonState::Hovered;
    if (widget.resize_edge)
        set_cursor(widget.resize_edge.value());
    decoration->spawn([](BasicDecoration* decoration)
        {
            decoration->input_state_updated();
        });
}

void msd::InputManager::widget_leave(Widget& widget)
{
    widget.state = ButtonState::Up;
    set_cursor(mir_resize_edge_none);
    decoration->spawn([](BasicDecoration* decoration)
        {
            decoration->input_state_updated();
        });
}

void msd::InputManager::widget_down(Widget& widget)
{
    widget.state = ButtonState::Down;
    decoration->spawn([](BasicDecoration* decoration)
        {
            decoration->input_state_updated();
        });
}

void msd::InputManager::widget_up(Widget& widget)
{
    if (widget.state == ButtonState::Down)
    {
        auto const input_ev = mir_event_get_input_event(latest_event().get());
        auto const event_timestamp = std::chrono::nanoseconds{mir_input_event_get_event_time(input_ev)};
        if (previous_up_timestamp > 0ns &&
            event_timestamp - previous_up_timestamp <= double_click_threshold)
        {
            // A double click happened

            if (widget.resize_edge && widget.resize_edge == mir_resize_edge_none)
            {
                // Widget is the titlebar, so we should toggle maximization
                decoration->spawn([](BasicDecoration* decoration)
                    {
                        decoration->request_toggle_maximize();
                    });
            }
        }

        if (widget.button)
        {
            switch (widget.button.value())
            {
            case ButtonFunction::Close:
                decoration->spawn([](BasicDecoration* decoration)
                    {
                        decoration->request_close();
                    });
                break;

            case ButtonFunction::Maximize:
                decoration->spawn([](BasicDecoration* decoration)
                    {
                        decoration->request_toggle_maximize();
                    });
                break;

            case ButtonFunction::Minimize:
                decoration->spawn([](BasicDecoration* decoration)
                    {
                        decoration->request_minimize();
                    });
                break;
            }
        }
    }
    widget.state = ButtonState::Hovered;
    decoration->spawn([](BasicDecoration* decoration)
        {
            decoration->input_state_updated();
        });
}

void msd::InputManager::widget_drag(Widget& widget)
{
    if (widget.state == ButtonState::Down)
    {
        if (widget.resize_edge)
        {
            auto edge = widget.resize_edge.value();

            if (edge == mir_resize_edge_none)
            {
                decoration->spawn([event = latest_event()](BasicDecoration* decoration)
                    {
                        decoration->request_move(mir_event_get_input_event(event.get()));
                    });
            }
            else
            {
                decoration->spawn([event = latest_event(), edge](BasicDecoration* decoration)
                    {
                        decoration->request_resize(mir_event_get_input_event(event.get()), edge);
                    });
            }
        }
        else if (widget.button)
        {
            decoration->spawn([event = latest_event()](BasicDecoration* decoration)
                {
                    decoration->request_move(mir_event_get_input_event(event.get()));
                });
        }
        widget.state = ButtonState::Up;
    }
    decoration->spawn([](BasicDecoration* decoration)
        {
            decoration->input_state_updated();
        });
}

void msd::InputManager::set_cursor(MirResizeEdge resize_edge)
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

    if (cursor_name != current_cursor_name)
    {
        current_cursor_name = cursor_name;
        decoration->spawn([cursor_name](BasicDecoration* decoration)
            {
                decoration->set_cursor(cursor_name);
            });
    }
}
