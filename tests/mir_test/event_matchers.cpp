/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include <ostream>
#include "mir_toolkit/event.h"

void PrintTo(MirInputEvent const& event, std::ostream* os)
{
    auto event_time = mir_input_event_get_event_time(&event);
    auto device_id = mir_input_event_get_device_id(&event);
    switch (mir_input_event_get_type(&event))
    {
    case mir_input_event_type_key:
        {
            char const key_event_actions[][7] = {"up", "down", "repeat"};
            auto key_event = mir_input_event_get_keyboard_event(&event);
            *os << "key_event(when=" << event_time << ", from=" << device_id << ", "
                << key_event_actions[mir_keyboard_event_action(key_event)]
                << ", code=" << mir_keyboard_event_key_code(key_event)
                << ", scan=" << mir_keyboard_event_scan_code(key_event) << ", modifiers=0x" << std::hex
                << mir_keyboard_event_modifiers(key_event) << std::dec << ')';
            break;
        }
    case mir_input_event_type_touch:
        {
            char const touch_actions[][7] = {"up", "down", "change"};
            char const touch_tooltypes[][8] = {"unknown", "finger", "stylus"};
            auto touch_event = mir_input_event_get_touch_event(&event);
            *os << "touch_event(when=" << event_time << ", from=" << device_id << ", touch = {";

            for (unsigned int index = 0, count = mir_touch_event_point_count(touch_event); index != count; ++index)
                *os << "{id=" << mir_touch_event_id(touch_event, index)
                    << ", action=" << touch_actions[mir_touch_event_action(touch_event, index)]
                    << ", tool=" << touch_tooltypes[mir_touch_event_tooltype(touch_event, index)]
                    << ", x=" << mir_touch_event_axis_value(touch_event, index, mir_touch_axis_x)
                    << ", y=" << mir_touch_event_axis_value(touch_event, index, mir_touch_axis_y)
                    << ", pressure=" << mir_touch_event_axis_value(touch_event, index, mir_touch_axis_pressure)
                    << ", major=" << mir_touch_event_axis_value(touch_event, index, mir_touch_axis_touch_major)
                    << ", minor=" << mir_touch_event_axis_value(touch_event, index, mir_touch_axis_touch_minor)
                    << ", size=" << mir_touch_event_axis_value(touch_event, index, mir_touch_axis_size) << '}';
            *os << ", modifiers=0x" << std::hex << mir_touch_event_modifiers(touch_event) << std::dec << ')';
            break;
        }
    case mir_input_event_type_pointer:
        {
            auto pointer_event = mir_input_event_get_pointer_event(&event);
            unsigned int button_state = 0;
            char const pointer_actions[][8] = {"up", "down", "repeat", "enter", "leave", "motion"};

            for (auto const a : {mir_pointer_button_primary, mir_pointer_button_secondary, mir_pointer_button_tertiary,
                 mir_pointer_button_back, mir_pointer_button_forward})
                button_state |= mir_pointer_event_button_state(pointer_event, a) ? a : 0;

            *os << "pointer_event(when=" << event_time << ", from=" << device_id << ", "
                << pointer_actions[mir_pointer_event_action(pointer_event)] << ", button_state=" << button_state
                << ", x=" << mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_x)
                << ", y=" << mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_y)
                << ", vscroll=" << mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_vscroll)
                << ", hscroll=" << mir_pointer_event_axis_value(pointer_event, mir_pointer_axis_hscroll)
                << ", modifiers=" << std::hex << mir_pointer_event_modifiers(pointer_event) << std::dec << ')';
            break;
        }
    }
}

void PrintTo(MirEvent const& event, std::ostream *os)
{
    switch (mir_event_get_type(&event))
    {
    case mir_event_type_surface:
        {
            auto surface_event = mir_event_get_surface_event(&event);
            *os << "surface_event(" << mir_surface_event_get_attribute(surface_event) << "="
                << mir_surface_event_get_attribute_value(surface_event)
                << ')';
            break;
        }
    case mir_event_type_resize:
        {
            auto resize_event = mir_event_get_resize_event(&event);
            *os << "resize_event(" << mir_resize_event_get_width(resize_event) << "x"
                << mir_resize_event_get_height(resize_event) << ")";
            break;
        }
    case mir_event_type_prompt_session_state_change:
        {
            char const states[][10] = {"stopped", "started", "suspended"};
            *os << "prompt_session_state_change_event("
                << states[mir_prompt_session_event_get_state(mir_event_get_prompt_session_event(&event))] << ")";
            break;
        }
    case mir_event_type_orientation:
        {
            *os << "prompt_session_state_change_event(";
            switch(mir_orientation_event_get_direction(mir_event_get_orientation_event(&event)))
            {
            case mir_orientation_normal:
                *os << "normal";
                break;
            case mir_orientation_left:
                *os << "left";
                break;
            case mir_orientation_right:
                *os << "right";
                break;
            case mir_orientation_inverted:
                *os << "inverted";
                break;
            default:
                *os << mir_orientation_event_get_direction(mir_event_get_orientation_event(&event));
                break;
            }

            *os << ")";
            break;
        }
    case mir_event_type_input:
        PrintTo(*mir_event_get_input_event(&event), os);
        break;
    case mir_event_type_keymap:
        {
            xkb_rule_names names;
            mir_keymap_event_get_rules(mir_event_get_keymap_event(&event), &names);
            *os << "keymap_event("
                << (names.rules?names.rules:"default rules") << ", "
                << (names.model?names.model:"default model") << ", "
                << (names.layout?names.layout:"default layout") << ", "
                << (names.variant?names.variant:"no variant") << ", "
                << (names.options?names.options:"no options") << ")";
            break;
        }
    case mir_event_type_close_surface:
        *os << "prompt_session_state_change_event()";
        break;
    default:
        break;
    }
}

void PrintTo(MirEvent const* event, std::ostream *os)
{
    if (event)
        PrintTo(*event, os);
    else
        *os << "nullptr";
}

