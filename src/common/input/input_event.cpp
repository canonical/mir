/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "mir_toolkit/input/input_event.h"

namespace
{
MirEvent const* old_ev_from_new(MirInputEvent const* ev)
{
    return reinterpret_cast<MirEvent const*>(ev);
}
/*MirEvent* old_ev_from_new(MirInputEvent* ev)
{
    return reinterpret_cast<MirEvent*>(ev);
}*/

MirInputEventType determine_motion_event_type(MirMotionEvent const& mev)
{
    switch (mev.action)
    {
    case mir_motion_action_hover_enter: 
    case mir_motion_action_hover_exit:
    case mir_motion_action_hover_move:
    case mir_motion_action_pointer_up:
    case mir_motion_action_pointer_down:
        return mir_input_event_type_pointer;
    default:
        return mir_input_event_type_touch;
    }
}

}

MirEventType mir_event_get_type(MirEvent const* ev)
{
    switch (ev->type)
    {
        case mir_event_type_key:
        case mir_event_type_motion:
            return mir_event_type_input;
        default:
            return ev->type;
    }
}

MirInputEvent* mir_event_get_input_event(MirEvent* ev)
{
    if (mir_event_get_type(ev) == mir_event_type_input)
        return reinterpret_cast<MirInputEvent*>(ev);
    return nullptr;
}

MirInputEventType mir_input_event_get_type(MirInputEvent const* ev)
{
    auto old_ev = old_ev_from_new(ev);
    switch (old_ev->type)
    {
    case mir_event_type_key:
        return mir_input_event_type_key;
    case mir_event_type_motion:
        return determine_motion_event_type(old_ev->motion);
    default:
        return mir_input_event_type_invalid;
    }
}

MirInputDeviceId mir_input_event_get_device_id(MirInputEvent const* ev)
{
    auto old_ev = old_ev_from_new(ev);
    switch (old_ev->type)
    {
        case mir_event_type_motion:
            return old_ev->motion.device_id;
        case mir_event_type_key:
            return old_ev->key.device_id;
        default:
            return -1;
    }
}

MirInputEventTime mir_input_event_get_event_time(MirInputEvent const* ev)
{
    auto old_ev = old_ev_from_new(ev);
    switch (old_ev->type)
    {
        case mir_event_type_motion:
            return old_ev->motion.event_time;
        case mir_event_type_key:
            return old_ev->key.event_time;
        default:
            // TODO: EC or assert?
            return -1;
    }
}

// Begin key ev stuff
// TODO: Move to new file?

MirKeyInputEvent* mir_input_event_get_key_input_event(MirInputEvent* ev)
{
    switch (mir_input_event_get_type(ev))
    {
        case mir_input_event_type_key:
            return reinterpret_cast<MirKeyInputEvent*>(ev);
        default:
            return nullptr;
    }
}

MirKeyInputEventAction mir_key_input_event_get_action(MirKeyInputEvent const* kev)
{
    auto const& old_kev = reinterpret_cast<MirEvent const*>(kev)->key;
    
    switch (old_kev.action)
    {
        case mir_key_action_down:
            if (old_kev.repeat_count != 0)
                return mir_key_input_event_action_repeat;
            else
                return mir_key_input_event_action_down;
        case mir_key_action_up:
            return mir_key_input_event_action_up;
        default:
            // TODO:? This means we got key_action_multiple which I dont think is 
            // actually emitted yet (and never will be as in the future this falls under text
            // event).
            return mir_key_input_event_action_down;
    }
}

xkb_keysym_t mir_key_input_event_get_key_code(MirKeyInputEvent const* kev)
{
    auto const& old_kev = reinterpret_cast<MirEvent const*>(kev)->key;
    return old_kev.key_code;
}

int mir_key_input_event_get_scan_code(MirKeyInputEvent const* kev)
{
    auto const& old_kev = reinterpret_cast<MirEvent const*>(kev)->key;
    return old_kev.scan_code;
}

MirKeyInputEventModifiers mir_key_input_event_get_modifiers(MirKeyInputEvent const* kev)
{
    auto const& old_kev = reinterpret_cast<MirEvent const*>(kev)->key;
    return static_cast<MirKeyInputEventModifiers>(old_kev.modifiers);
}
