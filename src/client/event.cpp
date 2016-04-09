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

#include <cstdlib>
#include <cstring>

namespace ml = mir::logging;

namespace
{
template <typename EventType>
void expect_event_type(EventType const* ev, MirEventType t) try
{
    if (ev->type() != t)
    {
        mir::log_critical("Expected " + mir::event_type_to_string(t) + " but event is of type " +
            mir::event_type_to_string(ev->type()));
        abort();
    }
} catch (...)
{
    abort();
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


MirEventType mir_event_get_type(MirEvent const* ev) try
{
    switch (ev->type())
    {
    case mir_event_type_key:
    case mir_event_type_motion:
        return mir_event_type_input;
    default:
        return ev->type();
    }
} catch (...)
{
    abort();
}

MirInputEvent const* mir_event_get_input_event(MirEvent const* ev) try
{
    if (ev->type() != mir_event_type_key && ev->type() != mir_event_type_motion)
    {
        mir::log_critical("Expected input event but event is of type " +
            mir::event_type_to_string(ev->type()));
        abort();
    }

    return ev->to_input();
} catch (...)
{
    abort();
}

MirSurfaceEvent const* mir_event_get_surface_event(MirEvent const* ev) try
{
    expect_event_type(ev, mir_event_type_surface);
    
    return ev->to_surface();
} catch (...)
{
    abort();
}

MirResizeEvent const* mir_event_get_resize_event(MirEvent const* ev) try
{
    expect_event_type(ev, mir_event_type_resize);
    
    return ev->to_resize();
} catch (...)
{
    abort();
}

MirPromptSessionEvent const* mir_event_get_prompt_session_event(MirEvent const* ev) try
{
    expect_event_type(ev, mir_event_type_prompt_session_state_change);
    
    return ev->to_prompt_session();
} catch (...)
{
    abort();
}

MirOrientationEvent const* mir_event_get_orientation_event(MirEvent const* ev) try
{
    expect_event_type(ev, mir_event_type_orientation);

    return ev->to_orientation();
} catch (...)
{
    abort();
}

MirCloseSurfaceEvent const* mir_event_get_close_surface_event(MirEvent const* ev) try
{
    expect_event_type(ev, mir_event_type_close_surface);

    return ev->to_close_surface();
} catch (...)
{
    abort();
}

MirKeymapEvent const* mir_event_get_keymap_event(MirEvent const* ev) try
{
    expect_event_type(ev, mir_event_type_keymap);

    return ev->to_keymap();
} catch (...)
{
    abort();
}

MirInputConfigurationEvent const* mir_event_get_input_configuration_event(MirEvent const* ev) try
{
    expect_event_type(ev, mir_event_type_input_configuration);

    return ev->to_input_configuration();
} catch (...)
{
    abort();
}

MirSurfaceOutputEvent const* mir_event_get_surface_output_event(MirEvent const* ev) try
{
    expect_event_type(ev, mir_event_type_surface_output);

    return ev->to_surface_output();
} catch (...)
{
    abort();
}

/* Surface event accessors */

MirSurfaceAttrib mir_surface_event_get_attribute(MirSurfaceEvent const* ev) try
{
    expect_event_type(ev, mir_event_type_surface);

    return ev->attrib();
} catch (...)
{
    abort();
}

int mir_surface_event_get_attribute_value(MirSurfaceEvent const* ev) try
{
    expect_event_type(ev, mir_event_type_surface);

    return ev->value();
} catch (...)
{
    abort();
}

/* Resize event accessors */

int mir_resize_event_get_width(MirResizeEvent const* ev) try
{
    expect_event_type(ev, mir_event_type_resize);
    return ev->width();
} catch (...)
{
    abort();
}

int mir_resize_event_get_height(MirResizeEvent const* ev) try
{
    expect_event_type(ev, mir_event_type_resize);
    return ev->height();
} catch (...)
{
    abort();
}

/* Prompt session event accessors */

MirPromptSessionState mir_prompt_session_event_get_state(MirPromptSessionEvent const* ev) try
{
    expect_event_type(ev, mir_event_type_prompt_session_state_change);
    return ev->new_state();
} catch (...)
{
    abort();
}

/* Orientation event accessors */

MirOrientation mir_orientation_event_get_direction(MirOrientationEvent const* ev) try
{
    expect_event_type(ev, mir_event_type_orientation);
    return ev->direction();
} catch (...)
{
    abort();
}

/* Keymap event accessors */

void mir_keymap_event_get_keymap_buffer(MirKeymapEvent const* ev, char const** buffer, size_t* length) try
{
    expect_event_type(ev, mir_event_type_keymap);

    *buffer = ev->buffer();
    *length = ev->size();
} catch (...)
{
    abort();
}

MirInputDeviceId mir_keymap_event_get_device_id(MirKeymapEvent const* ev) try
{
    expect_event_type(ev, mir_event_type_keymap);

    return ev->device_id();
} catch (...)
{
    abort();
}

/* Input configuration event accessors */

MirInputConfigurationAction mir_input_configuration_event_get_action(MirInputConfigurationEvent const* ev) try
{
    expect_event_type(ev, mir_event_type_input_configuration);
    return ev->action();
} catch (...)
{
    abort();
}

int64_t mir_input_configuration_event_get_time(MirInputConfigurationEvent const* ev) try
{
    expect_event_type(ev, mir_event_type_input_configuration);
    return ev->when().count();
} catch (...)
{
    abort();
}

MirInputDeviceId mir_input_configuration_event_get_device_id(MirInputConfigurationEvent const* ev) try
{
    expect_event_type(ev, mir_event_type_input_configuration);
    return ev->id();
} catch (...)
{
    abort();
}

/* Surface output event accessors */

int mir_surface_output_event_get_dpi(MirSurfaceOutputEvent const* ev) try
{
    expect_event_type(ev, mir_event_type_surface_output);
    return ev->dpi();
} catch (...)
{
    abort();
}

MirFormFactor mir_surface_output_event_get_form_factor(MirSurfaceOutputEvent const* ev) try
{
    expect_event_type(ev, mir_event_type_surface_output);
    return ev->form_factor();
} catch (...)
{
    abort();
}

float mir_surface_output_event_get_scale(MirSurfaceOutputEvent const* ev) try
{
    expect_event_type(ev, mir_event_type_surface_output);
    return ev->scale();
} catch (...)
{
    abort();
}

uint32_t mir_surface_output_event_get_output_id(MirSurfaceOutputEvent const *ev) try
{
    expect_event_type(ev, mir_event_type_surface_output);
    return ev->output_id();
} catch (...)
{
    abort();
}

// TODO: Until we opaquify the MirEvent structure and add
// a ref count ref is implemented as copy.
MirEvent const* mir_event_ref(MirEvent const* ev) try
{
    return new MirEvent(*ev);
} catch (...)
{
    abort();
}

void mir_event_unref(MirEvent const* ev)
{
    // Static assert that no one changes this implicit noexcept
    static_assert(noexcept(std::declval<MirEvent>), "");
    delete const_cast<MirEvent*>(ev);
}
