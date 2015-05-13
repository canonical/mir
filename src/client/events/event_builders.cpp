/*
 * Copyright © 2015 Canonical Ltd.
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
 * Author: Robert Carr <robert.carr@canonical.com>
 */

#include "mir/events/event_builders.h"
#include "mir/events/event_private.h"

#include <string.h>

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mf = mir::frontend;
namespace mev = mir::events;
namespace geom = mir::geometry;

namespace
{
    void delete_event(MirEvent *e) { delete e; }
    mir::EventUPtr make_event_uptr(MirEvent *e)
    {
        return mir::EventUPtr(e, delete_event);
    }
}

mir::EventUPtr mev::make_event(mf::SurfaceId const& surface_id, MirOrientation orientation)
{
    MirEvent *e = new MirEvent;
    memset(e, 0, sizeof (MirEvent));

    e->type = mir_event_type_orientation;
    e->orientation.surface_id = surface_id.as_value();
    e->orientation.direction = orientation;
    return make_event_uptr(e);
}

mir::EventUPtr mev::make_event(MirPromptSessionState state)
{
    MirEvent *e = new MirEvent;
    memset(e, 0, sizeof (MirEvent));

    e->type = mir_event_type_prompt_session_state_change;
    e->prompt_session.new_state = state;
    return make_event_uptr(e);
}

mir::EventUPtr mev::make_event(mf::SurfaceId const& surface_id, geom::Size const& size)
{
    MirEvent *e = new MirEvent;
    memset(e, 0, sizeof (MirEvent));

    e->type = mir_event_type_resize;
    e->resize.surface_id = surface_id.as_value();
    e->resize.width = size.width.as_int();
    e->resize.height = size.height.as_int();
    return make_event_uptr(e);
}

mir::EventUPtr mev::make_event(mf::SurfaceId const& surface_id, MirSurfaceAttrib attribute, int value)
{
    MirEvent *e = new MirEvent;
    memset(e, 0, sizeof (MirEvent));

    e->type = mir_event_type_surface;
    e->surface.id = surface_id.as_value();
    e->surface.attrib = attribute;
    e->surface.value = value;
    return make_event_uptr(e);
}

mir::EventUPtr mev::make_event(mf::SurfaceId const& surface_id)
{
    MirEvent *e = new MirEvent;
    memset(e, 0, sizeof (MirEvent));

    e->type = mir_event_type_close_surface;
    e->close_surface.surface_id = surface_id.as_value();
    return make_event_uptr(e);
}

mir::EventUPtr mev::make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    MirKeyboardAction action, xkb_keysym_t key_code,
    int scan_code, MirInputEventModifiers modifiers)
{
    MirEvent *e = new MirEvent;
    memset(e, 0, sizeof (MirEvent));

    e->type = mir_event_type_key;
    auto& kev = e->key;
    kev.device_id = device_id;
    kev.event_time = timestamp;
    kev.action = action;
    kev.key_code = key_code;
    kev.scan_code = scan_code;
    kev.modifiers = modifiers;

    return make_event_uptr(e);
}

namespace
{
// Never exposed in old event, so lets avoid leaking it in to a header now.
enum 
{
    AINPUT_SOURCE_CLASS_MASK = 0x000000ff,

    AINPUT_SOURCE_CLASS_BUTTON = 0x00000001,
    AINPUT_SOURCE_CLASS_POINTER = 0x00000002,
    AINPUT_SOURCE_CLASS_NAVIGATION = 0x00000004,
    AINPUT_SOURCE_CLASS_POSITION = 0x00000008,
    AINPUT_SOURCE_CLASS_JOYSTICK = 0x00000010
};
enum 
{
    AINPUT_SOURCE_UNKNOWN = 0x00000000,

    AINPUT_SOURCE_KEYBOARD = 0x00000100 | AINPUT_SOURCE_CLASS_BUTTON,
    AINPUT_SOURCE_DPAD = 0x00000200 | AINPUT_SOURCE_CLASS_BUTTON,
    AINPUT_SOURCE_GAMEPAD = 0x00000400 | AINPUT_SOURCE_CLASS_BUTTON,
    AINPUT_SOURCE_TOUCHSCREEN = 0x00001000 | AINPUT_SOURCE_CLASS_POINTER,
    AINPUT_SOURCE_MOUSE = 0x00002000 | AINPUT_SOURCE_CLASS_POINTER,
    AINPUT_SOURCE_STYLUS = 0x00004000 | AINPUT_SOURCE_CLASS_POINTER,
    AINPUT_SOURCE_TRACKBALL = 0x00010000 | AINPUT_SOURCE_CLASS_NAVIGATION,
    AINPUT_SOURCE_TOUCHPAD = 0x00100000 | AINPUT_SOURCE_CLASS_POSITION,
    AINPUT_SOURCE_JOYSTICK = 0x01000000 | AINPUT_SOURCE_CLASS_JOYSTICK,

    AINPUT_SOURCE_ANY = 0xffffff00
};
}

mir::EventUPtr mev::make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    MirInputEventModifiers modifiers)
{
    MirEvent *e = new MirEvent;
    memset(e, 0, sizeof (MirEvent));

    e->type = mir_event_type_motion;
    auto& mev = e->motion;
    mev.device_id = device_id;
    mev.event_time = timestamp;
    mev.modifiers = modifiers;
    mev.action = mir_motion_action_move;
    mev.source_id = AINPUT_SOURCE_TOUCHSCREEN;
    
    return make_event_uptr(e);
}

namespace
{
int const MIR_EVENT_ACTION_POINTER_INDEX_MASK = 0xff00;
int const MIR_EVENT_ACTION_POINTER_INDEX_SHIFT = 8;

void update_action_mask(MirMotionEvent &mev, MirTouchAction action)
{
    int new_mask = (mev.pointer_count - 1) << MIR_EVENT_ACTION_POINTER_INDEX_SHIFT;

    if (action == mir_touch_action_up)
        new_mask = mev.pointer_count == 1 ? mir_motion_action_up : (new_mask & MIR_EVENT_ACTION_POINTER_INDEX_MASK) |
                                                                       mir_motion_action_pointer_up;
    else if (action == mir_touch_action_down)
        new_mask = mev.pointer_count == 1 ? mir_motion_action_down : (new_mask & MIR_EVENT_ACTION_POINTER_INDEX_MASK) |
                                                                         mir_motion_action_pointer_down;
    else
    {
        // in case this is the second added touch and the primary touch point was an up or down action
        // we have to reset to pointer_up/pointer_down with index information (zero in this case)
        if (mev.action == mir_motion_action_up)
            mev.action = mir_motion_action_pointer_up;
        if (mev.action == mir_motion_action_down)
            mev.action = mir_motion_action_pointer_down;

        new_mask = mir_motion_action_move;
    }

    if (mev.action != mir_motion_action_move && new_mask != mir_motion_action_move)
    {
        BOOST_THROW_EXCEPTION(std::logic_error("Only one touch up/down may be reported per event"));
    }
    if (new_mask == mir_motion_action_move)
        return;
    mev.action = new_mask;
}

MirMotionToolType old_tooltype_from_new(MirTouchTooltype tooltype)
{
   switch (tooltype)
   {
   case mir_touch_tooltype_unknown:
       return mir_motion_tool_type_unknown;
   case mir_touch_tooltype_finger:
       return mir_motion_tool_type_finger;
   case mir_touch_tooltype_stylus:
       return mir_motion_tool_type_stylus;
   default:
       BOOST_THROW_EXCEPTION(std::logic_error("Invalid tooltype specified"));
   }
}
}

void mev::add_touch(MirEvent &event, MirTouchId touch_id, MirTouchAction action,
    MirTouchTooltype tooltype, float x_axis_value, float y_axis_value,
    float pressure_value, float touch_major_value, float touch_minor_value, float size_value)
{
    auto& mev = event.motion;
    auto& pc = mev.pointer_coordinates[mev.pointer_count++];
    pc.id = touch_id;
    pc.tool_type = old_tooltype_from_new(tooltype);
    pc.x = x_axis_value;
    pc.y = y_axis_value;
    pc.pressure = pressure_value;
    pc.touch_major = touch_major_value;
    pc.touch_minor = touch_minor_value;
    pc.size = size_value;

    update_action_mask(mev, action);
}

namespace
{
MirMotionAction old_action_from_pointer_action(MirPointerAction action, MirMotionButton button_state)
{
    switch (action)
    {
    case mir_pointer_action_button_up:
        return mir_motion_action_up;
    case mir_pointer_action_button_down:
        return mir_motion_action_down;
    case mir_pointer_action_enter:
        return mir_motion_action_hover_enter;
    case mir_pointer_action_leave:
        return mir_motion_action_hover_exit;
    case mir_pointer_action_motion:
        return button_state ? mir_motion_action_move : mir_motion_action_hover_move;
    default:
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid pointer action"));
    }
}
}

mir::EventUPtr mev::make_event(MirInputDeviceId device_id, std::chrono::nanoseconds timestamp,
    MirInputEventModifiers modifiers, MirPointerAction action,
    std::vector<MirPointerButton> const& buttons_pressed,
    float x_axis_value, float y_axis_value,
    float hscroll_value, float vscroll_value)
{
    MirEvent *e = new MirEvent;
    memset(e, 0, sizeof (MirEvent));

    e->type = mir_event_type_motion;
    auto& mev = e->motion;
    mev.device_id = device_id;
    mev.event_time = timestamp;
    mev.modifiers = modifiers;
    mev.source_id = AINPUT_SOURCE_MOUSE;

    int button_state = 0;
    for (auto button : buttons_pressed)
    {
    switch (button)
    {
    case mir_pointer_button_primary:
        button_state |= mir_motion_button_primary;
        break;
    case mir_pointer_button_secondary:
        button_state |= mir_motion_button_secondary;
        break;
    case mir_pointer_button_tertiary:
        button_state |= mir_motion_button_tertiary;
        break;
    case mir_pointer_button_back:
        button_state |= mir_motion_button_back;
        break;
    case mir_pointer_button_forward:
        button_state |= mir_motion_button_forward;
        break;
    }
    }
    mev.button_state = static_cast<MirMotionButton>(button_state);
    mev.action = old_action_from_pointer_action(action, mev.button_state);

    mev.pointer_count = 1;
    auto& pc = mev.pointer_coordinates[0];
    pc.x = x_axis_value;
    pc.y = y_axis_value;
    pc.hscroll = hscroll_value;
    pc.vscroll = vscroll_value;
    
    return make_event_uptr(e);
}

mir::EventUPtr mev::make_event(mf::SurfaceId const& surface_id, xkb_rule_names const& rules)
{
    MirEvent *e = new MirEvent;
    memset(e, 0, sizeof (MirEvent));

    e->type = mir_event_type_keymap;
    e->keymap.surface_id = surface_id.as_value();
    e->keymap.rules = rules;

    return make_event_uptr(e);
}
