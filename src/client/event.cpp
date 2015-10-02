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

#define MIR_LOG_COMPONENT "event-access"

#include "mir/event_type_to_string.h"
#include "mir/log.h"

#include "mir_toolkit/events/event.h"
#include "mir/events/event_private.h"

#include "mir_toolkit/events/surface_event.h"
#include "mir_toolkit/events/resize_event.h"
#include "mir_toolkit/events/prompt_session_event.h"
#include "mir_toolkit/events/orientation_event.h"

#include <stdlib.h>
#include <string.h>

namespace ml = mir::logging;

namespace
{
template <typename EventType>
void expect_event_type(EventType const* ev, MirEventType t)
{
    if (ev->type != t)
    {
        mir::log_critical("Expected " + mir::event_type_to_string(t) + " but event is of type " +
            mir::event_type_to_string(ev->type));
        abort();
    }
}
}

std::string mir::event_type_to_string(MirEventType t)
{
    switch (t)
    {
    case mir_event_type_key:
        return "mir_event_type_key";
    case mir_event_type_motion:
        return "mir_event_type_motion";
    case mir_event_type_surface:
        return "mir_event_type_surface";
    case mir_event_type_resize:
        return "mir_event_type_resize";
    case mir_event_type_prompt_session_state_change:
        return "mir_event_type_prompt_session_state_change";
    case mir_event_type_orientation:
        return "mir_event_type_orientation";
    case mir_event_type_close_surface:
        return "mir_event_type_close_surface";
    case mir_event_type_input:
        return "mir_event_type_input";
    default:
        abort();
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

MirInputEvent const* mir_event_get_input_event(MirEvent const* ev)
{
    if (ev->type != mir_event_type_key && ev->type != mir_event_type_motion)
    {
        mir::log_critical("Expected input event but event is of type " +
            mir::event_type_to_string(ev->type));
        abort();
    }

    return reinterpret_cast<MirInputEvent const*>(ev);
}

MirSurfaceEvent const* mir_event_get_surface_event(MirEvent const* ev)
{
    expect_event_type(ev, mir_event_type_surface);
    
    return &ev->surface;
}

MirResizeEvent const* mir_event_get_resize_event(MirEvent const* ev)
{
    expect_event_type(ev, mir_event_type_resize);
    
    return &ev->resize;
}

MirPromptSessionEvent const* mir_event_get_prompt_session_event(MirEvent const* ev)
{
    expect_event_type(ev, mir_event_type_prompt_session_state_change);
    
    return &ev->prompt_session;
}

MirOrientationEvent const* mir_event_get_orientation_event(MirEvent const* ev)
{
    expect_event_type(ev, mir_event_type_orientation);

    return &ev->orientation;
}

MirCloseSurfaceEvent const* mir_event_get_close_surface_event(MirEvent const* ev)
{
    expect_event_type(ev, mir_event_type_close_surface);

    return &ev->close_surface;
}

MirKeymapEvent const* mir_event_get_keymap_event(MirEvent const* ev)
{
    expect_event_type(ev, mir_event_type_keymap);

    return &ev->keymap;
}

MirInputConfigurationEvent const* mir_event_get_input_configuration_event(MirEvent const* ev)
{
    expect_event_type(ev, mir_event_type_input_configuration);

    return &ev->input_configuration;
}

MirSurfaceOutputEvent const* mir_event_get_surface_output_event(MirEvent const* ev)
{
    expect_event_type(ev, mir_event_type_surface_output);

    return &ev->surface_output;
}

/* Surface event accessors */

MirSurfaceAttrib mir_surface_event_get_attribute(MirSurfaceEvent const* ev)
{
    expect_event_type(ev, mir_event_type_surface);

    return ev->attrib;
}

int mir_surface_event_get_attribute_value(MirSurfaceEvent const* ev)
{
    expect_event_type(ev, mir_event_type_surface);

    return ev->value;
}

/* Resize event accessors */

int mir_resize_event_get_width(MirResizeEvent const* ev)
{
    expect_event_type(ev, mir_event_type_resize);
    return ev->width;
}

int mir_resize_event_get_height(MirResizeEvent const* ev)
{
    expect_event_type(ev, mir_event_type_resize);
    return ev->height;
}

/* Prompt session event accessors */

MirPromptSessionState mir_prompt_session_event_get_state(MirPromptSessionEvent const* ev)
{
    expect_event_type(ev, mir_event_type_prompt_session_state_change);
    return ev->new_state;
}

/* Orientation event accessors */

MirOrientation mir_orientation_event_get_direction(MirOrientationEvent const* ev)
{
    expect_event_type(ev, mir_event_type_orientation);
    return ev->direction;
}

/* Keymap event accessors */

void mir_keymap_event_get_rules(MirKeymapEvent const* ev, xkb_rule_names *out_names)
{
    expect_event_type(ev, mir_event_type_keymap);
    *out_names = ev->rules;
}

/* Input configuration event accessors */

MirInputConfigurationAction mir_input_configuration_event_get_action(MirInputConfigurationEvent const* ev)
{
    expect_event_type(ev, mir_event_type_input_configuration);
    return ev->action;
}

int64_t mir_input_configuration_event_get_time(MirInputConfigurationEvent const* ev)
{
    expect_event_type(ev, mir_event_type_input_configuration);
    return ev->when.count();
}

MirInputDeviceId mir_input_configuration_event_get_device_id(MirInputConfigurationEvent const* ev)
{
    expect_event_type(ev, mir_event_type_input_configuration);
    return ev->id;
}

/* Surface output event accessors */

int mir_surface_output_event_get_dpi(MirSurfaceOutputEvent const* ev)
{
    expect_event_type(ev, mir_event_type_surface_output);
    return ev->dpi;
}

MirFormFactor mir_surface_output_event_get_form_factor(MirSurfaceOutputEvent const* ev)
{
    expect_event_type(ev, mir_event_type_surface_output);
    return ev->form_factor;
}

float mir_surface_output_event_get_scale(MirSurfaceOutputEvent const* ev)
{
    expect_event_type(ev, mir_event_type_surface_output);
    return ev->scale;
}

// TODO: Until we opaquify the MirEvent structure and add
// a ref count ref is implemented as copy.
MirEvent const* mir_event_ref(MirEvent const* ev)
{
    MirEvent *new_ev = new MirEvent;
    memcpy(new_ev, ev, sizeof(MirEvent));
    return new_ev;
}

void mir_event_unref(MirEvent const* ev)
{
    delete const_cast<MirEvent*>(ev);
}
