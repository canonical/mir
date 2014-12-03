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

#include "mir_toolkit/event.h"

#include <stdlib.h>

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
    if (mir_event_get_type(ev) != mir_event_type_input)
        abort();

    return reinterpret_cast<MirInputEvent const*>(ev);
}

MirSurfaceEvent const* mir_event_get_surface_event(MirEvent const* ev)
{
    if (mir_event_get_type(ev) != mir_event_type_surface)
        abort();
    
    return reinterpret_cast<MirSurfaceEvent const*>(ev);
}

MirResizeEvent const* mir_event_get_resize_event(MirEvent const* ev)
{
    if (mir_event_get_type(ev) != mir_event_type_resize)
        abort();
    
    return reinterpret_cast<MirResizeEvent const*>(ev);
}

MirPromptSessionEvent const* mir_event_get_prompt_session_event(MirEvent const* ev)
{
    if (mir_event_get_type(ev) != mir_event_type_prompt_session_state_change)
        abort();
    
    return reinterpret_cast<MirPromptSessionEvent const*>(ev);
}

MirOrientationEvent const* mir_event_get_orientation_event(MirEvent const* ev)
{
    if (mir_event_get_type(ev) != mir_event_type_orientation)
        abort();
    
    return reinterpret_cast<MirOrientationEvent const*>(ev);
}

MirCloseSurfaceEvent const* mir_event_get_close_surface_event(MirEvent const* ev)
{
    if (mir_event_get_type(ev) != mir_event_type_close_surface)
        abort();
    
    return reinterpret_cast<MirCloseSurfaceEvent const*>(ev);
}
