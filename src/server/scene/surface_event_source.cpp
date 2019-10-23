/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
#include "mir/scene/output_properties_cache.h"

#include "mir/geometry/size.h"
#include "mir/geometry/rectangle.h"

#include <cstring>
#include <algorithm>

namespace ms = mir::scene;
namespace mev = mir::events;
namespace geom = mir::geometry;

ms::SurfaceEventSource::SurfaceEventSource(
    frontend::SurfaceId id,
    OutputPropertiesCache const& outputs,
    std::shared_ptr<frontend::EventSink> const& event_sink) :
    id(id),
    outputs{outputs},
    event_sink(event_sink)
{
}

void ms::SurfaceEventSource::content_resized_to(Surface const*, geometry::Size const& content_size)
{
    event_sink->handle_event(mev::make_event(id, content_size));
}

void ms::SurfaceEventSource::moved_to(Surface const* surface, geometry::Point const& top_left)
{
    auto new_output_properties = outputs.properties_for(geom::Rectangle{top_left, surface->size()});
    if (new_output_properties && (new_output_properties != last_output.lock()))
    {
        event_sink->handle_event(mev::make_event(
            id,
            new_output_properties->dpi,
            new_output_properties->scale,
            new_output_properties->refresh_rate,
            new_output_properties->form_factor,
            static_cast<uint32_t>(new_output_properties->id.as_value())
        ));
        last_output = new_output_properties;
    }
}

void ms::SurfaceEventSource::attrib_changed(Surface const*, MirWindowAttrib attrib, int value)
{
    event_sink->handle_event(mev::make_event(id, attrib, value));
}

void ms::SurfaceEventSource::orientation_set_to(Surface const*, MirOrientation orientation)
{
    event_sink->handle_event(mev::make_event(id, orientation));
}

void ms::SurfaceEventSource::client_surface_close_requested(Surface const*)
{
    event_sink->handle_event(mev::make_event(id));
}

void ms::SurfaceEventSource::keymap_changed(
    Surface const*,
    MirInputDeviceId device_id,
    std::string const& model,
    std::string const& layout,
    std::string const& variant,
    std::string const& options)
{
    event_sink->handle_event(mev::make_event(id, device_id, model, layout, variant, options));
}

void ms::SurfaceEventSource::placed_relative(Surface const*, geometry::Rectangle const& placement)
{
    event_sink->handle_event(mev::make_event(id, placement));
}

void ms::SurfaceEventSource::input_consumed(Surface const*, MirEvent const* event)
{
    auto ev = mev::clone_event(*event);
    mev::set_window_id(*ev, id.as_value());
    event_sink->handle_event(move(ev));
}

void ms::SurfaceEventSource::start_drag_and_drop(Surface const*, std::vector<uint8_t> const& handle)
{
    event_sink->handle_event(mev::make_start_drag_and_drop_event(id, handle));
}
