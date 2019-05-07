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

namespace
{
unsigned int const shift_states =
    mir_input_event_modifier_alt |
    mir_input_event_modifier_shift |
    mir_input_event_modifier_sym |
    mir_input_event_modifier_ctrl |
    mir_input_event_modifier_meta;

enum class PointerGesture
{
    none,
    moving,
    resizing
};
}

struct miral::Impl
{
    Impl(WindowManagerTools const& tools) : tools{tools} {}
    WindowManagerTools tools;

    PointerGesture pointer_gesture = PointerGesture::none;

    MirPointerButton pointer_gesture_button;
    miral::Window pointer_gesture_window;
    unsigned pointer_gesture_shift_keys = 0;
    MirResizeEdge resize_edge = mir_resize_edge_none;
    Point resize_top_left;
    Size resize_size;

    Point old_cursor{};

    bool begin_pointer_gesture(
        WindowInfo const& window_info, MirInputEvent const* input_event, PointerGesture gesture, MirResizeEdge edge);

    bool handle_pointer_event(MirPointerEvent const* event);
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

bool miral::MinimalWindowManager::handle_keyboard_event(MirKeyboardEvent const* /*event*/)
{
    return false;
}

bool miral::MinimalWindowManager::handle_touch_event(MirTouchEvent const* /*event*/)
{
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
    if (self->begin_pointer_gesture(window_info, input_event, PointerGesture::moving, mir_resize_edge_none))
    {
        return;
    }
    else
    {
        // TODO touch
    }
}

void miral::MinimalWindowManager::handle_request_resize(
    WindowInfo& window_info, MirInputEvent const* input_event, MirResizeEdge edge)
{
    if (self->begin_pointer_gesture(window_info, input_event, PointerGesture::resizing, edge))
    {
        return;
    }
    else
    {
        // TODO touch
    }
}

auto miral::MinimalWindowManager::confirm_inherited_move(WindowInfo const& window_info, Displacement movement)
-> Rectangle
{
    return {window_info.window().top_left()+movement, window_info.window().size()};
}

bool miral::Impl::begin_pointer_gesture(
    WindowInfo const& window_info, MirInputEvent const* input_event, PointerGesture gesture, MirResizeEdge edge)
{
    if (mir_input_event_get_type(input_event) != mir_input_event_type_pointer)
        return false;

    if (window_info.state() != mir_window_state_restored)
        return false;

    MirPointerEvent const* const pointer_event = mir_input_event_get_pointer_event(input_event);
    pointer_gesture = gesture;
    pointer_gesture_window = window_info.window();
    pointer_gesture_shift_keys = mir_pointer_event_modifiers(pointer_event) & shift_states;
    resize_top_left = pointer_gesture_window.top_left();
    resize_size = pointer_gesture_window.size();
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

bool miral::Impl::handle_pointer_event(MirPointerEvent const* event)
{
    auto const action = mir_pointer_event_action(event);
    auto const shift_keys = mir_pointer_event_modifiers(event) & shift_states;
    Point const cursor{
        mir_pointer_event_axis_value(event, mir_pointer_axis_x),
        mir_pointer_event_axis_value(event, mir_pointer_axis_y)};

    bool consumes_event = false;

    switch (pointer_gesture)
    {
    case PointerGesture::resizing:
        if (action == mir_pointer_action_motion &&
            shift_keys == pointer_gesture_shift_keys &&
            mir_pointer_event_button_state(event, pointer_gesture_button))
        {
            if (pointer_gesture_window)
            {
                if (tools.select_active_window(pointer_gesture_window) == pointer_gesture_window)
                {
                    auto const top_left = resize_top_left;
                    Rectangle const old_pos{top_left, resize_size};

                    auto movement = cursor - old_cursor;

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
                    tools.modify_window(pointer_gesture_window, modifications);
                    resize_top_left = new_pos;
                    resize_size = new_size;
                }
                else
                {
                    pointer_gesture = PointerGesture::none;
                }
            }
            else
            {
                pointer_gesture = PointerGesture::none;
            }

            consumes_event = true;
        }
        else
        {
            pointer_gesture = PointerGesture::none;
        }
        break;

    case PointerGesture::moving:
        if (action == mir_pointer_action_motion &&
            shift_keys == pointer_gesture_shift_keys &&
            mir_pointer_event_button_state(event, pointer_gesture_button))
        {
            if (auto const target = tools.window_at(old_cursor))
            {
                if (tools.select_active_window(target) == target)
                    tools.drag_active_window(cursor - old_cursor);
            }
            consumes_event = true;
        }
        else
            pointer_gesture = PointerGesture::none;
        break;

    default:
        break;
    }

    old_cursor = cursor;
    return consumes_event;
}
