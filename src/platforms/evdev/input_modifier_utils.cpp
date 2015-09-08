/*
 * Copyright © 2015 Canonical Ltd.
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

#include "input_modifier_utils.h"

#include "mir_toolkit/events/input/input_event.h"
#include "boost/throw_exception.hpp"

#include "linux/input.h"

#include <stdexcept>

namespace mie = mir::input::evdev;

MirPointerButton mie::to_pointer_button(int button)
{
    switch(button)
    {
    case BTN_LEFT: return mir_pointer_button_primary;
    case BTN_RIGHT: return mir_pointer_button_secondary;
    case BTN_MIDDLE: return mir_pointer_button_tertiary;
    case BTN_BACK: return mir_pointer_button_back;
    case BTN_FORWARD: return mir_pointer_button_forward;
    }
    BOOST_THROW_EXCEPTION(std::runtime_error("Invalid mouse button"));
}

MirInputEventModifier mie::to_modifier(int32_t scan_code)
{
    switch(scan_code)
    {
    case KEY_LEFTALT:
        return mir_input_event_modifier_alt_left;
    case KEY_RIGHTALT:
        return mir_input_event_modifier_alt_right;
    case KEY_RIGHTCTRL:
        return mir_input_event_modifier_ctrl_right;
    case KEY_LEFTCTRL:
        return mir_input_event_modifier_ctrl_left;
    case KEY_CAPSLOCK:
        return mir_input_event_modifier_caps_lock;
    case KEY_LEFTMETA:
        return mir_input_event_modifier_meta_left;
    case KEY_RIGHTMETA:
        return mir_input_event_modifier_meta_right;
    case KEY_SCROLLLOCK:
        return mir_input_event_modifier_scroll_lock;
    case KEY_NUMLOCK:
        return mir_input_event_modifier_num_lock;
    case KEY_LEFTSHIFT:
        return mir_input_event_modifier_shift_left;
    case KEY_RIGHTSHIFT:
        return mir_input_event_modifier_shift_right;
    default:
        return mir_input_event_modifier_none;
    }
}

MirInputEventModifiers mie::expand_modifiers(MirInputEventModifiers modifiers)
{
    if ((modifiers & mir_input_event_modifier_alt_left) || (modifiers & mir_input_event_modifier_alt_right))
        modifiers |= mir_input_event_modifier_alt;

    if ((modifiers & mir_input_event_modifier_ctrl_left) || (modifiers & mir_input_event_modifier_ctrl_right))
        modifiers |= mir_input_event_modifier_ctrl;

    if ((modifiers & mir_input_event_modifier_shift_left) || (modifiers & mir_input_event_modifier_shift_right))
        modifiers |= mir_input_event_modifier_shift;

    if ((modifiers & mir_input_event_modifier_meta_left) || (modifiers & mir_input_event_modifier_meta_right))
        modifiers |= mir_input_event_modifier_meta;

    return modifiers;
}


