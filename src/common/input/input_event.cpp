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

#include <assert.h>

namespace
{
MirEvent const* old_ev_from_new(MirInputEvent const* ev)
{
    return reinterpret_cast<MirEvent const*>(ev);
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

MirInputEvent const* mir_event_get_input_event(MirEvent  const* ev)
{
    assert(mir_event_get_type(ev) == mir_event_type_input);
    return reinterpret_cast<MirInputEvent const*>(ev);
}

MirInputEventType mir_input_event_get_type(MirInputEvent const* ev)
{
    auto old_ev = old_ev_from_new(ev);
    assert(old_ev->type == mir_event_type_key || old_ev->type == mir_event_type_motion);

    switch (old_ev->type)
    {
    case mir_event_type_key:
        return mir_input_event_type_key;
    case mir_event_type_motion:
        return mir_input_event_type_touch;
    default:
        return MirInputEventType();
    }
}

MirInputDeviceId mir_input_event_get_device_id(MirInputEvent const* ev)
{
    auto old_ev = old_ev_from_new(ev);
    assert(mir_event_get_type(old_ev) == mir_event_type_input);

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
    assert(mir_event_get_type(old_ev) == mir_event_type_input);

    switch (old_ev->type)
    {
        case mir_event_type_motion:
            return old_ev->motion.event_time;
        case mir_event_type_key:
            return old_ev->key.event_time;
        default:
            return -1;
    }
}

/* Key event accessors */

MirKeyInputEvent const* mir_input_event_get_key_input_event(MirInputEvent const* ev)
{
    assert(mir_input_event_get_type(ev) == mir_input_event_type_key);
    return reinterpret_cast<MirKeyInputEvent const*>(ev);
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
            // actually emitted yet (and never will be as in the future it would fall under text
            // event in the new model).
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

/* Touch event accessors */

MirTouchInputEvent const* mir_input_event_get_touch_input_event(MirInputEvent const* ev)
{
    assert(mir_input_event_get_type(ev) == mir_input_event_type_touch);
    return reinterpret_cast<MirTouchInputEvent const*>(ev);
}

unsigned int mir_touch_input_event_get_touch_count(MirTouchInputEvent const* event)
{
    auto const& old_mev = reinterpret_cast<MirEvent const*>(event)->motion;
    return old_mev.pointer_count;
}

MirTouchInputEventTouchId mir_touch_input_event_get_touch_id(MirTouchInputEvent const* event, size_t touch_index)
{
    auto const& old_mev = reinterpret_cast<MirEvent const*>(event)->motion;
    assert(touch_index <  old_mev.pointer_count);

    return old_mev.pointer_coordinates[touch_index].id;
}

MirTouchInputEventTouchAction mir_touch_input_event_get_touch_action(MirTouchInputEvent const* event, size_t touch_index)
{
    auto const& old_mev = reinterpret_cast<MirEvent const*>(event)->motion;
    assert(touch_index <  old_mev.pointer_count);
    
    auto masked_action = old_mev.action & MIR_EVENT_ACTION_MASK;
    size_t masked_index = (old_mev.action & MIR_EVENT_ACTION_POINTER_INDEX_MASK) >> MIR_EVENT_ACTION_POINTER_INDEX_SHIFT;

    switch (masked_action)
    {
    // For the next two cases we could assert pc=1...because a gesture must
    // be starting or ending.
    case mir_motion_action_down:
        return mir_touch_input_event_action_down;
    case mir_motion_action_up:
        return mir_touch_input_event_action_up;
    // We can't tell which touches have actually moved without tracking state
    // so we report all touchpoints as changed.
    case mir_motion_action_move:
    case mir_motion_action_hover_move:
        return mir_touch_input_event_action_change;
    // All touch points are handled at once so we don't know the index
    case mir_motion_action_cancel:
        return mir_touch_input_event_action_cancel;
    case mir_motion_action_pointer_down:
        if (touch_index == masked_index)
            return mir_touch_input_event_action_down;
        else
            return mir_touch_input_event_action_change;
    case mir_motion_action_pointer_up:
        if (touch_index == masked_index)
            return mir_touch_input_event_action_up;
        else
            return mir_touch_input_event_action_change;
    // TODO: How to deal with these?
    case mir_motion_action_outside:
    case mir_motion_action_scroll:
    case mir_motion_action_hover_enter:
    case mir_motion_action_hover_exit:
    default:
        return mir_touch_input_event_action_none;
    }
}

MirTouchInputEventTouchTooltype mir_touch_input_event_get_touch_tooltype(MirTouchInputEvent const* event,
    size_t touch_index)
{
    auto const& old_mev = reinterpret_cast<MirEvent const*>(event)->motion;
    assert(touch_index <  old_mev.pointer_count);

    switch (old_mev.pointer_coordinates[touch_index].tool_type)
    {
    case mir_motion_tool_type_finger:
        return mir_touch_input_tool_type_finger;
    case mir_motion_tool_type_stylus:
        return mir_touch_input_tool_type_stylus;
    case mir_motion_tool_type_mouse:
        return mir_touch_input_tool_type_mouse;
    case mir_motion_tool_type_eraser:
        return mir_touch_input_tool_type_eraser;
    case mir_motion_tool_type_unknown:
    default:
        return mir_touch_input_tool_type_unknown;
    }
}

float mir_touch_input_event_get_touch_axis_value(MirTouchInputEvent const* event,
    size_t touch_index, MirTouchInputEventTouchAxis axis)
{
    auto const& old_mev = reinterpret_cast<MirEvent const*>(event)->motion;
    assert(touch_index <  old_mev.pointer_count);

    auto const& old_pc = old_mev.pointer_coordinates[touch_index];
    switch (axis)
    {
    case mir_touch_input_event_touch_axis_x:
        return old_pc.x;
    case mir_touch_input_event_touch_axis_y:
        return old_pc.y;
    case mir_touch_input_event_touch_axis_pressure:
        return old_pc.pressure;
    case mir_touch_input_event_touch_axis_touch_major:
        return old_pc.touch_major;
    case mir_touch_input_event_touch_axis_touch_minor:
        return old_pc.touch_minor;
    default:
        return -1;
    }
}                                                                            
