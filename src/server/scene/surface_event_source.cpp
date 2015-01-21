/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#define MIR_INCLUDE_DEPRECATED_EVENT_HEADER 

#include "mir/scene/surface_event_source.h"

#include "mir/geometry/size.h"

#include <cstring>
#include <algorithm>

namespace ms = mir::scene;
namespace geom = mir::geometry;

ms::SurfaceEventSource::SurfaceEventSource(
    frontend::SurfaceId id,
    std::shared_ptr<frontend::EventSink> const& event_sink) :
    id(id),
    event_sink(event_sink)
{
}

void ms::SurfaceEventSource::resized_to(geom::Size const& size)
{
    MirEvent e;
    memset(&e, 0, sizeof e);
    e.type = mir_event_type_resize;
    e.resize.surface_id = id.as_value();
    e.resize.width = size.width.as_int();
    e.resize.height = size.height.as_int();
    event_sink->handle_event(e);
}


void ms::SurfaceEventSource::attrib_changed(MirSurfaceAttrib attrib, int value)
{
    MirEvent e;

    // This memset is not really required. However it does avoid some
    // harmless uninitialized memory reads that valgrind will complain
    // about, due to gaps in MirEvent.
    memset(&e, 0, sizeof e);

    e.type = mir_event_type_surface;
    e.surface.id = id.as_value();
    e.surface.attrib = attrib;
    e.surface.value = value;

    event_sink->handle_event(e);
}

void ms::SurfaceEventSource::orientation_set_to(MirOrientation orientation)
{
    MirEvent e;
    memset(&e, 0, sizeof e);

    e.type = mir_event_type_orientation;
    e.orientation.surface_id = id.as_value();
    e.orientation.direction = orientation;

    event_sink->handle_event(e);
}

void ms::SurfaceEventSource::client_surface_close_requested()
{
    MirEvent e;
    memset(&e, 0, sizeof e);

    e.type = mir_event_type_close_surface;
    e.close_surface.surface_id = id.as_value();

    event_sink->handle_event(e);
}
