/*
 * Copyright © 2019 Canonical Ltd.
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
 *
 * Authored By: William Wold <william.wold@canonical.com>
 */

#include "input.h"
#include "window.h"
#include "threadsafe_access.h"
#include "basic_decoration.h"

#include "mir/scene/surface.h"
#include "mir/scene/null_surface_observer.h"
#include "mir_toolkit/cursors.h"

namespace ms = mir::scene;
namespace mi = mir::input;
namespace geom = mir::geometry;
namespace msh = mir::shell;
namespace msd = mir::shell::decoration;

struct msd::InputManager::Observer
    : public ms::NullSurfaceObserver
{
    Observer(InputManager* manager)
        : manager{manager}
    {
    }

    /// Overrides from NullSurfaceObserver
    /// @{
    void input_consumed(ms::Surface const*, MirEvent const* event) override
    {
        if (mir_event_get_type(event) != mir_event_type_input)
            return;
        MirInputEvent const* const input_ev = mir_event_get_input_event(event);
        auto const timestamp = std::chrono::nanoseconds{mir_input_event_get_event_time(input_ev)};
        switch (mir_input_event_get_type(input_ev))
        {
        case mir_input_event_type_pointer:
        {
            MirPointerEvent const* const pointer_ev = mir_input_event_get_pointer_event(input_ev);
            switch (mir_pointer_event_action(pointer_ev))
            {
            case mir_pointer_action_button_up:
            case mir_pointer_action_button_down:
            case mir_pointer_action_motion:
            case mir_pointer_action_enter:
            {
                geom::Point const location{
                    mir_pointer_event_axis_value(pointer_ev, mir_pointer_axis_x),
                    mir_pointer_event_axis_value(pointer_ev, mir_pointer_axis_y)};
                bool pressed = mir_pointer_event_button_state(pointer_ev, mir_pointer_button_primary);
                manager->pointer_event(timestamp, location, pressed);
            }   break;

            case mir_pointer_action_leave:
            {
                manager->pointer_leave(timestamp);
            }   break;

            case mir_pointer_actions:
                break;
            }
        }   break;

        case mir_input_event_type_touch:
        {
            MirTouchEvent const* const touch_ev = mir_input_event_get_touch_event(input_ev);
            for (unsigned int i = 0; i < mir_touch_event_point_count(touch_ev); i++)
            {
                auto const id = mir_touch_event_id(touch_ev, i);
                switch (mir_touch_event_action(touch_ev, i))
                {
                case mir_touch_action_down:
                case mir_touch_action_change:
                {
                    geom::Point const location{
                        mir_touch_event_axis_value(touch_ev, i, mir_touch_axis_x),
                        mir_touch_event_axis_value(touch_ev, i, mir_touch_axis_y)};
                    manager->touch_event(id, timestamp, location);
                }   break;
                case mir_touch_action_up:
                {
                    manager->touch_up(id, timestamp);
                }   break;
                case mir_touch_actions:
                    break;
                }
            }

        }   break;

        case mir_input_event_type_key:
        case mir_input_event_types:
            break;
        }
    }
    /// @}

    InputManager* const manager;
};

msd::InputManager::InputManager(
    std::shared_ptr<ms::Surface> const& decoration_surface,
    WindowState const& window_state,
    std::shared_ptr<ThreadsafeAccess<BasicDecoration>> const& decoration)
    : decoration_surface{decoration_surface},
      observer{std::make_shared<Observer>(this)},
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
    decoration_surface->add_observer(observer);
}

msd::InputManager::~InputManager()
{
    decoration_surface->remove_observer(observer);
}

void msd::InputManager::update_window_state(WindowState const& window_state)
{
    std::lock_guard<std::mutex> lock{mutex};

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
            widget->rect = resize_edge_rect(window_state, widget->resize_edge.value());
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
    std::lock_guard<std::mutex> lock{mutex};
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

auto msd::InputManager::resize_edge_rect(WindowState const& window_state, MirResizeEdge resize_edge) -> geom::Rectangle
{
    switch (resize_edge)
    {
    case mir_resize_edge_none:
        return window_state.titlebar_rect();

    case mir_resize_edge_north:
    {
        auto rect = window_state.titlebar_rect();
        rect.size.height = window_state.static_geometry().border_height;
        return rect;
    }

    case mir_resize_edge_south:
        return window_state.bottom_border_rect();

    case mir_resize_edge_west:
        return window_state.left_border_rect();

    case mir_resize_edge_east:
        return window_state.right_border_rect();

    case mir_resize_edge_northwest:
        return {{}, window_state.static_geometry().resize_corner_size};

    case mir_resize_edge_northeast:
    {
        geom::Point top_left{
            as_x(window_state.window_size().width) -
                as_delta(window_state.static_geometry().resize_corner_size.width),
            0};
        return {top_left, window_state.static_geometry().resize_corner_size};
    }

    case mir_resize_edge_southwest:
    {
        geom::Point top_left{
            0,
            as_y(window_state.window_size().height) -
                as_delta(window_state.static_geometry().resize_corner_size.height)};
        return {top_left, window_state.static_geometry().resize_corner_size};
    }

    case mir_resize_edge_southeast:
    {
        geom::Point top_left{
            as_x(window_state.window_size().width) -
                as_delta(window_state.static_geometry().resize_corner_size.width),
            as_y(window_state.window_size().height) -
                as_delta(window_state.static_geometry().resize_corner_size.height)};
        return {top_left, window_state.static_geometry().resize_corner_size};
    }

    default: return {};
    }
}

void msd::InputManager::pointer_event(std::chrono::nanoseconds timestamp, geom::Point location, bool pressed)
{
    std::lock_guard<std::mutex> lock{mutex};
    last_timestamp = timestamp;
    if (!pointer)
    {
        pointer = Device{location, pressed};
        process_enter(pointer.value());
    }
    if (pointer.value().location != location)
    {
        pointer.value().location = location;
        if (pointer.value().pressed)
            process_drag(pointer.value());
        else
            process_move(pointer.value());
    }
    if (pointer.value().pressed != pressed)
    {
        pointer.value().pressed = pressed;
        if (pressed)
            process_down(pointer.value());
        else
            process_up(pointer.value());
    }
}

void msd::InputManager::pointer_leave(std::chrono::nanoseconds timestamp)
{
    std::lock_guard<std::mutex> lock{mutex};
    last_timestamp = timestamp;
    if (pointer)
        process_leave(pointer.value());
    pointer = std::experimental::nullopt;
}

void msd::InputManager::touch_event(int32_t id, std::chrono::nanoseconds timestamp, geom::Point location)
{
    std::lock_guard<std::mutex> lock{mutex};
    last_timestamp = timestamp;
    auto device = touches.find(id);
    if (device == touches.end())
    {
        device = touches.insert(std::make_pair(id, Device{location, false})).first;
        process_enter(device->second);
        device->second.pressed = true;
        process_down(device->second);
    }
    if (device->second.location != location)
    {
        device->second.location = location;
        process_drag(device->second);
    }
}

void msd::InputManager::touch_up(int32_t id, std::chrono::nanoseconds timestamp)
{
    std::lock_guard<std::mutex> lock{mutex};
    last_timestamp = timestamp;
    auto device = touches.find(id);
    if (device != touches.end())
    {
        process_up(device->second);
        process_leave(device->second);
        touches.erase(device);
    }
}

void msd::InputManager::process_enter(Device& device)
{
    device.active_widget = widget_at(device.location);
    if (device.active_widget)
        widget_enter(*device.active_widget.value());
}

void msd::InputManager::process_leave(Device& device)
{
    if (device.active_widget)
    {
        widget_leave(*device.active_widget.value());
        device.active_widget = std::experimental::nullopt;
    }
}

void msd::InputManager::process_down(Device& device)
{
    if (device.active_widget)
    {
        widget_down(*device.active_widget.value());
    }
}

void msd::InputManager::process_up(Device& device)
{
    if (device.active_widget)
    {
        widget_up(*device.active_widget.value());
    }
}

void msd::InputManager::process_move(Device& device)
{
    auto const new_widget = widget_at(device.location);
    if (new_widget != device.active_widget)
    {
        if (device.active_widget)
            widget_leave(*device.active_widget.value());

        device.active_widget = new_widget;

        if (device.active_widget)
            widget_enter(*device.active_widget.value());
    }
}

void msd::InputManager::process_drag(Device& device)
{
    auto const new_widget = widget_at(device.location);

    if (device.active_widget)
    {
        widget_drag(*device.active_widget.value());
        if (new_widget != device.active_widget)
            widget_leave(*device.active_widget.value());
    }

    bool const enter = new_widget && (device.active_widget != new_widget);
    device.active_widget = new_widget;
    if (enter)
        widget_enter(*device.active_widget.value());
}

auto msd::InputManager::widget_at(geom::Point location) -> std::experimental::optional<std::shared_ptr<Widget>>
{
    for (auto const& widget : widgets)
    {
        if (widget->rect.contains(location))
            return widget;
    }
    return std::experimental::nullopt;
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
                decoration->spawn([timestamp = last_timestamp](BasicDecoration* decoration)
                    {
                        decoration->request_move(timestamp);
                    });
            }
            else
            {
                decoration->spawn([timestamp = last_timestamp, edge](BasicDecoration* decoration)
                    {
                        decoration->request_resize(timestamp, edge);
                    });
            }
        }
        else if (widget.button)
        {
            decoration->spawn([timestamp = last_timestamp](BasicDecoration* decoration)
                {
                    decoration->request_move(timestamp);
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
