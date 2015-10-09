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

#include "mir/events/event_builders.h"
#include "mir/scene/surface_event_source.h"
#include "mir/scene/surface.h"
#include "output_properties_cache.h"

#include "mir/geometry/size.h"
#include "mir/geometry/rectangle.h"

#include <cstring>
#include <algorithm>

namespace ms = mir::scene;
namespace mev = mir::events;
namespace geom = mir::geometry;

ms::SurfaceEventSource::SurfaceEventSource(
    frontend::SurfaceId id,
    Surface const& surface,
    OutputPropertiesCache const& outputs,
    std::shared_ptr<frontend::EventSink> const& event_sink) :
    id(id),
    surface{surface},
    outputs{outputs},
    event_sink(event_sink)
{
}

void ms::SurfaceEventSource::resized_to(geom::Size const& size)
{
    event_sink->handle_event(*mev::make_event(id, size));
}

void ms::SurfaceEventSource::moved_to(geom::Point const& top_left)
{
    auto new_output_properties = outputs.properties_for(geom::Rectangle{top_left, surface.size()});
    if (new_output_properties && (new_output_properties != last_output.lock()))
    {
        event_sink->handle_event(*mev::make_event(
            id,
            new_output_properties->dpi,
            new_output_properties->scale,
            new_output_properties->form_factor));
        last_output = new_output_properties;
    }
}

void ms::SurfaceEventSource::attrib_changed(MirSurfaceAttrib attrib, int value)
{
    event_sink->handle_event(*mev::make_event(id, attrib, value));
}

void ms::SurfaceEventSource::orientation_set_to(MirOrientation orientation)
{
    event_sink->handle_event(*mev::make_event(id, orientation));
}

void ms::SurfaceEventSource::client_surface_close_requested()
{
    event_sink->handle_event(*mev::make_event(id));
}

void ms::SurfaceEventSource::keymap_changed(xkb_rule_names const& names)
{
    event_sink->handle_event(*mev::make_event(id, names));
}
