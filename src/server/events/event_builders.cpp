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

#define MIR_INCLUDE_DEPRECATED_EVENT_HEADER

#include "mir/events/event_builders.h"

#include <string.h>

namespace mf = mir::frontend;
namespace mev = mir::events;
namespace geom = mir::geometry;

std::shared_ptr<MirEvent> mev::make_event(mf::SurfaceId const& surface_id, MirOrientation orientation)
{
    MirEvent *e = new MirEvent;
    memset(e, 0, sizeof (MirEvent));

    e->type = mir_event_type_orientation;
    e->orientation.surface_id = surface_id.as_value();
    e->orientation.direction = orientation;
    return std::shared_ptr<MirEvent>(e);
}

std::shared_ptr<MirEvent> mev::make_event(MirPromptSessionState state)
{
    MirEvent *e = new MirEvent;
    memset(e, 0, sizeof (MirEvent));

    e->type = mir_event_type_prompt_session_state_change;
    e->prompt_session.new_state = state;
    return std::shared_ptr<MirEvent>(e);
}

std::shared_ptr<MirEvent> mev::make_event(mf::SurfaceId const& surface_id, geom::Size const& size)
{
    MirEvent *e = new MirEvent;
    memset(e, 0, sizeof (MirEvent));

    e->type = mir_event_type_resize;
    e->resize.surface_id = surface_id.as_value();
    e->resize.width = size.width.as_int();
    e->resize.height = size.height.as_int();
    return std::shared_ptr<MirEvent>(e);
}

std::shared_ptr<MirEvent> mev::make_event(mf::SurfaceId const& surface_id, MirSurfaceAttrib attribute, int value)
{
    MirEvent *e = new MirEvent;
    memset(e, 0, sizeof (MirEvent));

    e->type = mir_event_type_surface;
    e->surface.id = surface_id.as_value();
    e->surface.attrib = attribute;
    e->surface.value = value;
    return std::shared_ptr<MirEvent>(e);
}

std::shared_ptr<MirEvent> mev::make_event(mf::SurfaceId const& surface_id)
{
    MirEvent *e = new MirEvent;
    memset(e, 0, sizeof (MirEvent));

    e->type = mir_event_type_close_surface;
    e->close_surface.surface_id = surface_id.as_value();
    return std::shared_ptr<MirEvent>(e);
}
