/*
 * Copyright © 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include <miral/minimal_window_manager.h>

#include <linux/input.h>
#include <gmpxx.h>

namespace
{
unsigned int const shift_states =
    mir_input_event_modifier_alt |
    mir_input_event_modifier_shift |
    mir_input_event_modifier_sym |
    mir_input_event_modifier_ctrl |
    mir_input_event_modifier_meta;

enum class Gesture
{
    none,
    pointer_moving,
    pointer_resizing,
    touch_moving,
    touch_resizing,
};

auto touch_center(MirTouchEvent const* event) -> mir::geometry::Point
{
    auto const count = mir_touch_event_point_count(event);

    long total_x = 0;
    long total_y = 0;

    for (auto i = 0U; i != count; ++i)
    {
        total_x += mir_touch_event_axis_value(event, i, mir_touch_axis_x);
        total_y += mir_touch_event_axis_value(event, i, mir_touch_axis_y);
    }

    return {total_x/count, total_y/count};
}
}

struct miral::MinimalWindowManager::Impl
{
    Impl(WindowManagerTools const& tools) : tools{tools} {}
    WindowManagerTools tools;

    Gesture gesture = Gesture::none;
    MirPointerButton pointer_gesture_button;
    miral::Window gesture_window;
    unsigned gesture_shift_keys = 0;
    MirResizeEdge resize_edge = mir_resize_edge_none;
    Point resize_top_left;
    Size resize_size;
    Point old_cursor{};
    Point old_touch{};

    bool begin_pointer_gesture(
        WindowInfo const& window_info, MirInputEvent const* input_event, Gesture gesture, MirResizeEdge edge);

    bool begin_touch_gesture(
        WindowInfo const& window_info, MirInputEvent const* input_event, Gesture gesture, MirResizeEdge edge);

    bool handle_pointer_event(MirPointerEvent const* event);

    bool handle_touch_event(MirTouchEvent const* event);

    void apply_resize_by(Displacement movement);
};

miral::MinimalWindowManager::MinimalWindowManager(WindowManagerTools const& tools):
    tools{tools},
    self{new Impl{tools}}
{
}

miral::MinimalWindowManager::~MinimalWindowManager()
{
    delete self;
}

auto miral::MinimalWindowManager::place_new_window(
    ApplicationInfo const& /*app_info*/, WindowSpecification const& requested_specification)
    -> WindowSpecification
{
    return requested_specification;
}

void miral::MinimalWindowManager::handle_window_ready(WindowInfo& window_info)
{
    if (window_info.can_be_active())
    {
        tools.select_active_window(window_info.window());
    }
}

void miral::MinimalWindowManager::handle_modify_window(
    WindowInfo& window_info, miral::WindowSpecification const& modifications)
{
    tools.modify_window(window_info, modifications);
}

void miral::MinimalWindowManager::handle_raise_window(WindowInfo& window_info)
{
    tools.select_active_window(window_info.window());
}

auto miral::MinimalWindowManager::confirm_placement_on_display(
    WindowInfo const& /*window_info*/, MirWindowState /*new_state*/, Rectangle const& new_placement)
    -> Rectangle
{
    return new_placement;
}

bool miral::MinimalWindowManager::handle_keyboard_event(MirKeyboardEvent const* event)
{
    auto const action = mir_keyboard_event_action(event);
    auto const shift_state = mir_keyboard_event_modifiers(event) & shift_states;

    if (action == mir_keyboard_action_down && shift_state == mir_input_event_modifier_alt)
    {
        switch (mir_keyboard_event_scan_code(event))
        {
        case KEY_F4:
            tools.ask_client_to_close(tools.active_window());
            return true;

        case KEY_TAB:
            tools.focus_next_application();
            return true;

        case KEY_GRAVE:
            tools.focus_next_within_application();
            return true;

        default:;
        }
    }

    if (action == mir_keyboard_action_down &&
        shift_state == (mir_input_event_modifier_alt | mir_input_event_modifier_shift))
    {
        switch (mir_keyboard_event_scan_code(event))
        {
        case KEY_TAB:
            tools.focus_prev_application();
            return true;

        case KEY_GRAVE:
            tools.focus_prev_within_application();
            return true;

        default:;
        }
    }

    return false;
}

bool miral::MinimalWindowManager::handle_touch_event(MirTouchEvent const* event)
{
    if (self->handle_touch_event(event))
    {
        return true;
    }

    return false;
}

bool miral::MinimalWindowManager::handle_pointer_event(MirPointerEvent const* event)
{
    return self->handle_pointer_event(event);
}

void miral::MinimalWindowManager::handle_request_drag_and_drop(WindowInfo& /*window_info*/)
{
    // TODO
}

void miral::MinimalWindowManager::handle_request_move(WindowInfo& window_info, MirInputEvent const* input_event)
{
    if (begin_pointer_move(window_info, input_event))
    {
        return;
    }
    else if (begin_touch_move(window_info, input_event))
    {
        return;
    }
}

bool miral::MinimalWindowManager::begin_pointer_move(WindowInfo const& window_info, MirInputEvent const* input_event)
{
    return self->begin_pointer_gesture(window_info, input_event, Gesture::pointer_moving, mir_resize_edge_none);
}

bool miral::MinimalWindowManager::begin_touch_move(WindowInfo const& window_info, MirInputEvent const* input_event)
{
    return self->begin_touch_gesture(window_info, input_event, Gesture::touch_moving, mir_resize_edge_none);
}

void miral::MinimalWindowManager::handle_request_resize(
    WindowInfo& window_info, MirInputEvent const* input_event, MirResizeEdge edge)
{
    if (begin_pointer_resize(window_info, input_event, edge))
    {
        return;
    }
    else if (begin_touch_resize(window_info, input_event, edge))
    {
        return;
    }
}

bool miral::MinimalWindowManager::begin_touch_resize(
    WindowInfo const& window_info, MirInputEvent const* input_event, MirResizeEdge const& edge)
{
    return self->begin_touch_gesture(window_info, input_event, Gesture::touch_resizing, edge);
}

bool miral::MinimalWindowManager::begin_pointer_resize(
    WindowInfo const& window_info, MirInputEvent const* input_event, MirResizeEdge const& edge)
{
    return self->begin_pointer_gesture(window_info, input_event, Gesture::pointer_resizing, edge);
}

auto miral::MinimalWindowManager::confirm_inherited_move(WindowInfo const& window_info, Displacement movement)
-> Rectangle
{
    return {window_info.window().top_left()+movement, window_info.window().size()};
}

void miral::MinimalWindowManager::advise_focus_gained(WindowInfo const& window_info)
{
    tools.raise_tree(window_info.window());
}

bool miral::MinimalWindowManager::Impl::begin_pointer_gesture(
    WindowInfo const& window_info, MirInputEvent const* input_event, Gesture gesture_, MirResizeEdge edge)
{
    if (mir_input_event_get_type(input_event) != mir_input_event_type_pointer)
        return false;

    if (window_info.state() != mir_window_state_restored)
        return false;

    MirPointerEvent const* const pointer_event = mir_input_event_get_pointer_event(input_event);
    gesture = gesture_;
    gesture_window = window_info.window();
    gesture_shift_keys = mir_pointer_event_modifiers(pointer_event) & shift_states;
    resize_top_left = gesture_window.top_left();
    resize_size = gesture_window.size();
    resize_edge = edge;

    for (auto button : {mir_pointer_button_primary, mir_pointer_button_secondary, mir_pointer_button_tertiary})
    {
        if (mir_pointer_event_button_state(pointer_event, button))
        {
            pointer_gesture_button = button;
            break;
        }
    }

    return true;
}

bool miral::MinimalWindowManager::Impl::begin_touch_gesture(
    WindowInfo const& window_info,
    MirInputEvent const* input_event,
    Gesture gesture_,
    MirResizeEdge edge)
{
    if (mir_input_event_get_type(input_event) != mir_input_event_type_touch)
        return false;

    if (window_info.state() != mir_window_state_restored)
        return false;

    MirTouchEvent const* const touch_event = mir_input_event_get_touch_event(input_event);
    gesture = gesture_;
    gesture_window = window_info.window();
    gesture_shift_keys = mir_touch_event_modifiers(touch_event) & shift_states;
    resize_top_left = gesture_window.top_left();
    resize_size = gesture_window.size();
    resize_edge = edge;

    return true;
}

bool miral::MinimalWindowManager::Impl::handle_pointer_event(MirPointerEvent const* event)
{
    auto const action = mir_pointer_event_action(event);
    auto const shift_keys = mir_pointer_event_modifiers(event) & shift_states;
    Point const new_cursor{
        mir_pointer_event_axis_value(event, mir_pointer_axis_x),
        mir_pointer_event_axis_value(event, mir_pointer_axis_y)};

    bool consumes_event = false;

    switch (gesture)
    {
    case Gesture::pointer_resizing:
        if (action == mir_pointer_action_motion &&
            shift_keys == gesture_shift_keys &&
            mir_pointer_event_button_state(event, pointer_gesture_button))
        {
            apply_resize_by(new_cursor - old_cursor);
            consumes_event = true;
        }
        else
        {
            gesture = Gesture::none;
        }
        break;

    case Gesture::pointer_moving:
        if (action == mir_pointer_action_motion &&
            shift_keys == gesture_shift_keys &&
            mir_pointer_event_button_state(event, pointer_gesture_button))
        {
            if (gesture_window &&
                tools.select_active_window(gesture_window) == gesture_window)
            {
                tools.drag_active_window(new_cursor - old_cursor);
                consumes_event = true;
            }
            else
            {
                gesture = Gesture::none;
            }
        }
        else
        {
            gesture = Gesture::none;
        }
        break;

    default:
        break;
    }

    if (!consumes_event && action == mir_pointer_action_button_down)
    {
        if (auto const window = tools.window_at(new_cursor))
        {
            tools.select_active_window(window);
        }
    }

    old_cursor = new_cursor;
    return consumes_event;
}

bool miral::MinimalWindowManager::Impl::handle_touch_event(MirTouchEvent const* event)
{
    bool consumes_event = false;
    auto const new_touch = touch_center(event);
    auto const count = mir_touch_event_point_count(event);
    auto const shift_keys = mir_touch_event_modifiers(event) & shift_states;

    bool is_drag = true;
    for (auto i = 0U; i != count; ++i)
    {
        switch (mir_touch_event_action(event, i))
        {
        case mir_touch_action_up:
        case mir_touch_action_down:
            is_drag = false;
            // Falls through
        default:
            continue;
        }
    }

    switch (gesture)
    {
    case Gesture::touch_resizing:
        if (is_drag && gesture_shift_keys == shift_keys)
        {
            if (gesture_window &&
                tools.select_active_window(gesture_window) == gesture_window)
            {
                apply_resize_by(new_touch - old_touch);
                consumes_event = true;
            }
            else
            {
                gesture = Gesture::none;
            }
        }
        else
        {
            gesture = Gesture::none;
        }
        break;

    case Gesture::touch_moving:
        if (is_drag && gesture_shift_keys == shift_keys)
        {
            if (gesture_window &&
                tools.select_active_window(gesture_window) == gesture_window)
            {
                tools.drag_active_window(new_touch - old_touch);
                consumes_event = true;
            }
            else
            {
                gesture = Gesture::none;
            }
        }
        else
        {
            gesture = Gesture::none;
        }
        break;

    default:
        break;
    }

    if (!consumes_event && count == 1 && mir_touch_event_action(event, 0) == mir_touch_action_down)
    {
        if (auto const window = tools.window_at(new_touch))
        {
            tools.select_active_window(window);
        }
    }

    old_touch = new_touch;
    return consumes_event;
}

void miral::MinimalWindowManager::Impl::apply_resize_by(Displacement movement)
{
    if (gesture_window &&
        tools.select_active_window(gesture_window) == gesture_window)
    {
        auto const top_left = resize_top_left;
        Rectangle const old_pos{top_left, resize_size};

        auto new_width = old_pos.size.width;
        auto new_height = old_pos.size.height;

        if (resize_edge & mir_resize_edge_east)
            new_width = old_pos.size.width + movement.dx;

        if (resize_edge & mir_resize_edge_west)
            new_width = old_pos.size.width - movement.dx;

        if (resize_edge & mir_resize_edge_north)
            new_height = old_pos.size.height - movement.dy;

        if (resize_edge & mir_resize_edge_south)
            new_height = old_pos.size.height + movement.dy;

        Size new_size{new_width, new_height};

        Point new_pos = top_left;

        if (resize_edge & mir_resize_edge_west)
            new_pos.x = top_left.x + movement.dx;

        if (resize_edge & mir_resize_edge_north)
            new_pos.y = top_left.y + movement.dy;

        WindowSpecification modifications;
        modifications.top_left() = new_pos;
        modifications.size() = new_size;
        tools.modify_window(gesture_window, modifications);
        resize_top_left = new_pos;
        resize_size = new_size;
    }
    else
    {
        gesture = Gesture::none;
    }
}
