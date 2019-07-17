/*
 * Copyright © 2015-2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/event_printer.h"

#include "mir/events/event_private.h"
#include "mir/events/surface_placement_event.h"
#include "mir/logging/input_timestamp.h"

std::ostream& mir::operator<<(std::ostream& out, MirInputEventModifier modifier)
{
    bool none = true;
#define PRINT_FLAG(base,name) if ((modifier & base ## _ ## name) == base ## _ ## name)\
    {\
        if (!none) out << ' ';\
        out << # name;\
        none= false;\
    }

    PRINT_FLAG(mir_input_event_modifier, alt);
    PRINT_FLAG(mir_input_event_modifier, alt_right);
    PRINT_FLAG(mir_input_event_modifier, alt_left);
    PRINT_FLAG(mir_input_event_modifier, shift);
    PRINT_FLAG(mir_input_event_modifier, shift_left);
    PRINT_FLAG(mir_input_event_modifier, shift_right);
    PRINT_FLAG(mir_input_event_modifier, sym);
    PRINT_FLAG(mir_input_event_modifier, function);
    PRINT_FLAG(mir_input_event_modifier, ctrl);
    PRINT_FLAG(mir_input_event_modifier, ctrl_left);
    PRINT_FLAG(mir_input_event_modifier, ctrl_right);
    PRINT_FLAG(mir_input_event_modifier, meta);
    PRINT_FLAG(mir_input_event_modifier, meta_left);
    PRINT_FLAG(mir_input_event_modifier, meta_right);
    PRINT_FLAG(mir_input_event_modifier, caps_lock);
    PRINT_FLAG(mir_input_event_modifier, num_lock);
    PRINT_FLAG(mir_input_event_modifier, scroll_lock);
    if (none) out << "none";
#undef PRINT_FLAG
    return out;
}

#define PRINT(base,name) case base ## _ ## name: return out << #name;

std::ostream& mir::operator<<(std::ostream& out, MirKeyboardAction action)
{
    switch (action)
    {
    PRINT(mir_keyboard_action,up);
    PRINT(mir_keyboard_action,down);
    PRINT(mir_keyboard_action,repeat);
    default:
        return out << static_cast<int>(action) << "<INVALID>";
    }
}

std::ostream& mir::operator<<(std::ostream& out, MirTouchAction action)
{
    switch (action)
    {
    PRINT(mir_touch_action,up);
    PRINT(mir_touch_action,down);
    PRINT(mir_touch_action,change);
    default:
        return out << static_cast<int>(action) << "<INVALID>";
    }
}

std::ostream& mir::operator<<(std::ostream& out, MirTouchTooltype type)
{
    switch (type)
    {
    PRINT(mir_touch_tooltype,finger);
    PRINT(mir_touch_tooltype,stylus);
    PRINT(mir_touch_tooltype,unknown);
    default:
        return out << static_cast<int>(type) << "<INVALID>";
    }
}

std::ostream& mir::operator<<(std::ostream& out, MirPointerAction action)
{
    switch (action)
    {
    PRINT(mir_pointer_action,button_up);
    PRINT(mir_pointer_action,button_down);
    PRINT(mir_pointer_action,enter);
    PRINT(mir_pointer_action,leave);
    PRINT(mir_pointer_action,motion);
    default:
        return out << static_cast<int>(action) << "<INVALID>";
    }
}

std::ostream& mir::operator<<(std::ostream& out, MirPromptSessionState state)
{
    switch (state)
    {
    PRINT(mir_prompt_session_state,started);
    PRINT(mir_prompt_session_state,stopped);
    PRINT(mir_prompt_session_state,suspended);
    default:
        return out << static_cast<int>(state) << "<INVALID>";
    }
}

std::ostream& mir::operator<<(std::ostream& out, MirOrientation orientation)
{
    switch (orientation)
    {
    PRINT(mir_orientation,right);
    PRINT(mir_orientation,inverted);
    PRINT(mir_orientation,left);
    PRINT(mir_orientation,normal);
    default:
        return out << static_cast<int>(orientation) << "<INVALID>";
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
std::ostream& mir::operator<<(std::ostream& out, MirSurfaceAttrib attribute)
{
    switch (attribute)
    {
    PRINT(mir_surface_attrib,type);
    PRINT(mir_surface_attrib,dpi);
    PRINT(mir_surface_attrib,focus);
    PRINT(mir_surface_attrib,state);
    PRINT(mir_surface_attrib,visibility);
    PRINT(mir_surface_attrib,swapinterval);
    PRINT(mir_surface_attrib,preferred_orientation);
    default:
        return out << static_cast<int>(attribute) << "<INVALID>";
    }
}

std::ostream& mir::operator<<(std::ostream& out, MirSurfaceFocusState state)
{
    switch (state)
    {
    PRINT(mir_surface,focused);
    PRINT(mir_surface,unfocused);
    default:
        return out << static_cast<int>(state) << "<INVALID>";
    }
}

std::ostream& mir::operator<<(std::ostream& out, MirSurfaceVisibility state)
{
    switch (state)
    {
    PRINT(mir_surface_visibility,exposed);
    PRINT(mir_surface_visibility,occluded);
    default:
        return out << static_cast<int>(state) << "<INVALID>";
    }
}

std::ostream& mir::operator<<(std::ostream& out, MirSurfaceType type)
{
    switch (type)
    {
    PRINT(mir_surface_type,normal);
    PRINT(mir_surface_type,utility);
    PRINT(mir_surface_type,dialog);
    PRINT(mir_surface_type,gloss);
    PRINT(mir_surface_type,freestyle);
    PRINT(mir_surface_type,menu);
    PRINT(mir_surface_type,inputmethod);
    PRINT(mir_surface_type,satellite);
    PRINT(mir_surface_type,tip);
    default:
        return out << static_cast<int>(type) << "<INVALID>";
    }
}

std::ostream& mir::operator<<(std::ostream& out, MirSurfaceState state)
{
    switch (state)
    {
    PRINT(mir_surface_state,unknown);
    PRINT(mir_surface_state,restored);
    PRINT(mir_surface_state,minimized);
    PRINT(mir_surface_state,maximized);
    PRINT(mir_surface_state,vertmaximized);
    PRINT(mir_surface_state,fullscreen);
    PRINT(mir_surface_state,horizmaximized);
    PRINT(mir_surface_state,hidden);
    default:
        return out << static_cast<int>(state) << "<INVALID>";
    }
}
#pragma GCC diagnostic pop

std::ostream& mir::operator<<(std::ostream& out, MirWindowAttrib attribute)
{
    switch (attribute)
    {
    PRINT(mir_window_attrib,type);
    PRINT(mir_window_attrib,dpi);
    PRINT(mir_window_attrib,focus);
    PRINT(mir_window_attrib,state);
    PRINT(mir_window_attrib,visibility);
    PRINT(mir_window_attrib,swapinterval);
    PRINT(mir_window_attrib,preferred_orientation);
    default:
        return out << static_cast<int>(attribute) << "<INVALID>";
    }
}

std::ostream& mir::operator<<(std::ostream& out, MirWindowFocusState state)
{
    switch (state)
    {
    PRINT(mir_window_focus_state,focused);
    PRINT(mir_window_focus_state,unfocused);
    default:
        return out << static_cast<int>(state) << "<INVALID>";
    }
}

std::ostream& mir::operator<<(std::ostream& out, MirWindowVisibility state)
{
    switch (state)
    {
    PRINT(mir_window_visibility,exposed);
    PRINT(mir_window_visibility,occluded);
    default:
        return out << static_cast<int>(state) << "<INVALID>";
    }
}

std::ostream& mir::operator<<(std::ostream& out, MirWindowType type)
{
    switch (type)
    {
    PRINT(mir_window_type,normal);
    PRINT(mir_window_type,utility);
    PRINT(mir_window_type,dialog);
    PRINT(mir_window_type,gloss);
    PRINT(mir_window_type,freestyle);
    PRINT(mir_window_type,menu);
    PRINT(mir_window_type,inputmethod);
    PRINT(mir_window_type,satellite);
    PRINT(mir_window_type,tip);
    default:
        return out << static_cast<int>(type) << "<INVALID>";
    }
}

std::ostream& mir::operator<<(std::ostream& out, MirWindowState state)
{
    switch (state)
    {
    PRINT(mir_window_state,unknown);
    PRINT(mir_window_state,restored);
    PRINT(mir_window_state,minimized);
    PRINT(mir_window_state,maximized);
    PRINT(mir_window_state,vertmaximized);
    PRINT(mir_window_state,fullscreen);
    PRINT(mir_window_state,horizmaximized);
    PRINT(mir_window_state,hidden);
    PRINT(mir_window_state,attached);
    default:
        return out << static_cast<int>(state) << "<INVALID>";
    }
}

#undef PRINT

std::ostream& mir::operator<<(std::ostream& out, MirInputEvent const& event)
{
    auto event_time = mir::logging::input_timestamp(std::chrono::nanoseconds(mir_input_event_get_event_time(&event)));
    auto device_id = mir_input_event_get_device_id(&event);
    auto window_id = event.window_id();
    switch (mir_input_event_get_type(&event))
    {
    case mir_input_event_type_key:
        {
            auto key_event = mir_input_event_get_keyboard_event(&event);
            return out << "key_event(when=" << event_time << ", from=" << device_id << ", window_id=" << window_id << ", "
                << mir_keyboard_event_action(key_event)
                << ", code=" << mir_keyboard_event_key_code(key_event)
                << ", scan=" << mir_keyboard_event_scan_code(key_event) << ", modifiers=" << std::hex
                << static_cast<MirInputEventModifier>(mir_keyboard_event_modifiers(key_event)) << std::dec << ')';
        }
    case mir_input_event_type_touch:
        {
            auto touch_event = mir_input_event_get_touch_event(&event);
            out << "touch_event(when=" << event_time << ", from=" << device_id << ", window_id=" << window_id << ", touch = {";

            for (unsigned int index = 0, count = mir_touch_event_point_count(touch_event); index != count; ++index)
                out << "{id=" << mir_touch_event_id(touch_event, index)
                    << ", action=" << mir_touch_event_action(touch_event, index)
                    << ", tool=" << mir_touch_event_tooltype(touch_event, index)
                    << ", x=" << mir_touch_event_axis_value(touch_event, index, mir_touch_axis_x)
                    << ", y=" << mir_touch_event_axis_value(touch_event, index, mir_touch_axis_y)
                    << ", pressure=" << mir_touch_event_axis_value(touch_event, index, mir_touch_axis_pressure)
                    << ", major=" << mir_touch_event_axis_value(touch_event, index, mir_touch_axis_touch_major)
                    << ", minor=" << mir_touch_event_axis_value(touch_event, index, mir_touch_axis_touch_minor)
                    << ", size=" << mir_touch_event_axis_value(touch_event, index, mir_touch_axis_size) << '}';

            return out << ", modifiers=" << static_cast<MirInputEventModifier>(mir_touch_event_modifiers(touch_event)) << ')';
        }
    case mir_input_event_type_pointer:
        {
            auto pointer_event = mir_input_event_get_pointer_event(&event);
            unsigned int button_state = 0;

            for (auto const a : {mir_pointer_button_primary, mir_pointer_button_secondary, mir_pointer_button_tertiary,
                 mir_pointer_button_back, mir_pointer_button_forward})
                button_state |= mir_pointer_event_button_state(pointer_event, a) ? a : 0;

            return out << "pointer_event(when=" << event_time << ", from=" << device_id << ", window_id=" << window_id << ", "
                << mir_pointer_event_action(pointer_event) << ", button_state=" << button_state
                << ", x=" << mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_x)
                << ", y=" << mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_y)
                << ", dx=" << mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_relative_x)
                << ", dy=" << mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_relative_y)
                << ", vscroll=" << mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_vscroll)
                << ", hscroll=" << mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_hscroll)
                << ", modifiers=" << static_cast<MirInputEventModifier>(mir_pointer_event_modifiers(pointer_event)) << ')';
        }
    default:
        return out << "<INVALID>";
    }
}

std::ostream& mir::operator<<(std::ostream& out, MirPromptSessionEvent const& event)
{
    return out << "prompt_session_event(state=" << mir_prompt_session_event_get_state(&event) << ")";
}

std::ostream& mir::operator<<(std::ostream& out, MirResizeEvent const& event)
{
    return out << "resize_event(state=" << mir_resize_event_get_width(&event) << "x"
        << mir_resize_event_get_height(&event) << ")";
}

std::ostream& mir::operator<<(std::ostream& out, MirOrientationEvent const& event)
{
    return out << "orientation_event(" << mir_orientation_event_get_direction(&event) << ")";
}

std::ostream& mir::operator<<(std::ostream& out, MirKeymapEvent const& event)
{
    return out << "keymap_event(blob, device_id=" << mir_keymap_event_get_device_id(&event) << ")";
}

std::ostream& mir::operator<<(std::ostream& out, MirCloseWindowEvent const&)
{
    return out << "close_window_event()";
}

std::ostream& mir::operator<<(std::ostream& out, MirWindowEvent const& event)
{
    out << "window_event("
        << mir_window_event_get_attribute(&event)<< '=';
    auto value = mir_window_event_get_attribute_value(&event);
    switch (mir_window_event_get_attribute(&event))
    {
    case mir_window_attrib_type:
        out << static_cast<MirWindowType>(value);
        break;
    case mir_window_attrib_state:
        out << static_cast<MirWindowState>(value);
        break;
    case mir_window_attrib_swapinterval:
        out << value;
        break;
    case mir_window_attrib_focus:
        out << static_cast<MirWindowFocusState>(value);
        break;
    case mir_window_attrib_dpi:
        out << value;
        break;
    case mir_window_attrib_visibility:
        out << static_cast<MirWindowVisibility>(value);
        break;
    case mir_window_attrib_preferred_orientation:
        out << value;
        break;
    default:
        break;
    }

    return out << ')';
}

std::ostream& mir::operator<<(std::ostream& out, MirWindowOutputEvent const& event)
{
    return out << "window_output_event({"
               << mir_window_output_event_get_dpi(&event) << ", "
               << mir_window_output_event_get_form_factor(&event) << ", "
               << mir_window_output_event_get_scale(&event) << ", "
               << mir_window_output_event_get_refresh_rate(&event) << ", "
               << mir_window_output_event_get_output_id(&event) << "})";
}

std::ostream& mir::operator<<(std::ostream& out, MirWindowPlacementEvent const& event)
{
    auto const& placement = event.placement();
    return out << "window_placement_event({"
               << placement.left << ", "
               << placement.top << ", "
               << placement.width << ", "
               << placement.height << "})";
}

std::ostream& mir::operator<<(std::ostream& out, MirInputDeviceStateEvent const& event)
{
    auto window_id = event.window_id();
    out << "input_device_state(ts="
        << mir_input_device_state_event_time(&event)
        << ", window_id=" << window_id
        << ", mod=" << MirInputEventModifier(mir_input_device_state_event_modifiers(&event))
        << ", btns=" << mir_input_device_state_event_pointer_buttons(&event)
        << ", x=" << mir_input_device_state_event_pointer_axis(&event, mir_pointer_axis_x)
        << ", y=" << mir_input_device_state_event_pointer_axis(&event, mir_pointer_axis_y)
        << " [";

    for (size_t size = mir_input_device_state_event_device_count(&event), index = 0; index != size; ++index)
    {
        out << mir_input_device_state_event_device_id(&event, index) 
            << " btns=" << mir_input_device_state_event_device_pointer_buttons(&event, index)
            << " pressed=(";
        auto key_count = mir_input_device_state_event_device_pressed_keys_count(&event, index);
        for (uint32_t i = 0; i < key_count; i++)
        {
            out << mir_input_device_state_event_device_pressed_keys_for_index(&event, index, i);
            if (i + 1 < key_count)
                out << ", ";
        }
        out << ")";
        if (index + 1 < size)
            out << ", ";
    }
    return out << "]";
}

#define PRINT_EVENT(type) case mir_event_type_ ## type : return out << *mir_event_get_ ## type ## _event(&event);

std::ostream& mir::operator<<(std::ostream& out, MirEvent const& event)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    auto type = mir_event_get_type(&event);
    switch (type)
    {
        PRINT_EVENT(window);
        PRINT_EVENT(resize);
        PRINT_EVENT(orientation);
        PRINT_EVENT(close_surface);
        PRINT_EVENT(input);
        PRINT_EVENT(input_device_state);
        PRINT_EVENT(keymap);
        PRINT_EVENT(window_placement);
        PRINT_EVENT(window_output);
    case mir_event_type_prompt_session_state_change:
        return out << *mir_event_get_prompt_session_event(&event);
    default:
        return out << static_cast<int>(type) << "<INVALID>";
    }
#pragma GCC diagnostic pop
}

