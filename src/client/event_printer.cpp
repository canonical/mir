/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
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

#undef PRINT

std::ostream& mir::operator<<(std::ostream& out, MirInputEvent const& event)
{
    auto event_time = mir::logging::input_timestamp(std::chrono::nanoseconds(mir_input_event_get_event_time(&event)));
    auto device_id = mir_input_event_get_device_id(&event);
    switch (mir_input_event_get_type(&event))
    {
    case mir_input_event_type_key:
        {
            auto key_event = mir_input_event_get_keyboard_event(&event);
            return out << "key_event(when=" << event_time << ", from=" << device_id << ", "
                << mir_keyboard_event_action(key_event)
                << ", code=" << mir_keyboard_event_key_code(key_event)
                << ", scan=" << mir_keyboard_event_scan_code(key_event) << ", modifiers=" << std::hex
                << mir_keyboard_event_modifiers(key_event) << ')';
        }
    case mir_input_event_type_touch:
        {
            auto touch_event = mir_input_event_get_touch_event(&event);
            out << "touch_event(when=" << event_time << ", from=" << device_id << ", touch = {";

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

            return out << ", modifiers=" << mir_touch_event_modifiers(touch_event) << ')';
        }
    case mir_input_event_type_pointer:
        {
            auto pointer_event = mir_input_event_get_pointer_event(&event);
            unsigned int button_state = 0;

            for (auto const a : {mir_pointer_button_primary, mir_pointer_button_secondary, mir_pointer_button_tertiary,
                 mir_pointer_button_back, mir_pointer_button_forward})
                button_state |= mir_pointer_event_button_state(pointer_event, a) ? a : 0;

            return out << "pointer_event(when=" << event_time << ", from=" << device_id << ", "
                << mir_pointer_event_action(pointer_event) << ", button_state=" << button_state
                << ", x=" << mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_x)
                << ", y=" << mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_y)
                << ", dx=" << mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_relative_x)
                << ", dy=" << mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_relative_y)
                << ", vscroll=" << mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_vscroll)
                << ", hscroll=" << mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_hscroll)
                << ", modifiers=" << mir_pointer_event_modifiers(pointer_event) << ')';
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
    xkb_rule_names names;
    mir_keymap_event_get_rules(&event, &names);
    return out << "keymap_event("
        << (names.rules?names.rules:"default rules") << ", "
        << (names.model?names.model:"default model") << ", "
        << (names.layout?names.layout:"default layout") << ", "
        << (names.variant?names.variant:"no variant") << ", "
        << (names.options?names.options:"no options") << ")";
}

std::ostream& mir::operator<<(std::ostream& out, MirCloseSurfaceEvent const&)
{
    return out << "close_surface_event()";
}

std::ostream& mir::operator<<(std::ostream& out, MirSurfaceEvent const& event)
{
    out << "surface_event("
        << mir_surface_event_get_attribute(&event)<< '=';
    auto value = mir_surface_event_get_attribute_value(&event);
    switch (mir_surface_event_get_attribute(&event))
    {
    case mir_surface_attrib_type:
        out << static_cast<MirSurfaceType>(value);
        break;
    case mir_surface_attrib_state:
        out << static_cast<MirSurfaceState>(value);
        break;
    case mir_surface_attrib_swapinterval:
        out << value;
        break;
    case mir_surface_attrib_focus:
        out << static_cast<MirSurfaceFocusState>(value);
        break;
    case mir_surface_attrib_dpi:
        out << value;
        break;
    case mir_surface_attrib_visibility:
        out << static_cast<MirSurfaceVisibility>(value);
        break;
    case mir_surface_attrib_preferred_orientation:
        out << value;
        break;
    default:
        break;
    }

    return out << ')';
}

#define PRINT_EVENT(type) case mir_event_type_ ## type : return out << *mir_event_get_ ## type ## _event(&event);

std::ostream& mir::operator<<(std::ostream& out, MirEvent const& event)
{
    auto type = mir_event_get_type(&event);
    switch (type)
    {
        PRINT_EVENT(surface);
        PRINT_EVENT(resize);
        PRINT_EVENT(orientation);
        PRINT_EVENT(close_surface);
        PRINT_EVENT(input);
        PRINT_EVENT(keymap);
    case mir_event_type_prompt_session_state_change:
        return out << *mir_event_get_prompt_session_event(&event);
    default:
        return out << static_cast<int>(type) << "<INVALID>";
    }
}

