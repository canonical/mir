
/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "wp_viewporter.h"
#include "mir/geometry/forward.h"
#include "mir/wayland/protocol_error.h"
#include "mir/wayland/weak.h"
#include "wl_surface.h"

#include <utility>

namespace mf = mir::frontend;
namespace geom = mir::geometry;

namespace
{
class ViewporterInstance : public mir::wayland::Viewporter
{
public:
    explicit ViewporterInstance(wl_resource* resource)
        : Viewporter(resource, Version<1>{})
    {
    }

private:
    void get_viewport(wl_resource* id, wl_resource* surface) override
    {
        auto surf = mf::WlSurface::from(surface);
        try
        {
            new mf::Viewport(id, surf);
        }
        catch (std::logic_error const&)
        {
            // We get a std::logic_error if the surface already had a viewport; translate to protocol exception here
            throw mir::wayland::ProtocolError{
                resource,
                Viewporter::Error::viewport_exists,
                "Surface already has a viewport associated"};
        }
    }
};
}

mf::WpViewporter::WpViewporter(wl_display* display)
    : Global(display, Version<1>{})
{
}

void mf::WpViewporter::bind(wl_resource* new_resource)
{
    new ViewporterInstance(new_resource);
}

mf::Viewport::Viewport(wl_resource* new_viewport, WlSurface* surface)
    : wayland::Viewport(new_viewport, Version<1>{})
{
    surface->associate_viewport(wayland::make_weak<Viewport>(this));
    this->surface = wayland::make_weak<wayland::Surface>(surface);
}

mf::Viewport::~Viewport()
{
}

auto mf::Viewport::changed_since_last_resolve() -> bool
{
    return viewport_changed;
}

namespace
{
auto as_int_if_exact(double d) -> std::optional<int>
{
    if (std::trunc(d) == d && d < std::numeric_limits<int>::max())
    {
        return static_cast<int>(d);
    }
    return {};
}

auto as_int_size_if_exact(geom::SizeD size) -> std::optional<geom::Size>
{
    auto width = as_int_if_exact(size.width.as_value());
    auto height = as_int_if_exact(size.height.as_value());

    if (width && height)
    {
        return geom::Size{*width, *height};
    }
    return {};
}
}

auto mf::Viewport::resolve_viewport(int32_t scale, geom::Size buffer_size) const
    -> std::pair<geom::RectangleD, geom::Size>
{
    auto const src_bounds =
        [&]()
        {
            auto const entire_buffer = geom::RectangleD{{0, 0}, geom::SizeD{buffer_size}};
            if (source)
            {
                auto raw_source = source.value();
                /* Viewport coordinates are in buffer coordinates *after* applying transformation and scale.
                 * That means this rectangle needs to be scaled. (And have buffer transform applied, when we support that)
                 */
                geom::RectangleD source_in_buffer_coords{
                    {raw_source.left().as_value() * scale, raw_source.top().as_value() * scale},
                    raw_source.size * scale
                };
                if (!entire_buffer.contains(source_in_buffer_coords))
                {
                    throw wayland::ProtocolError{
                        resource,
                        wayland::Viewport::Error::out_of_buffer,
                        "Source viewport (%f, %f), (%f × %f) is not entirely within buffer (%i × %i)",
                        source_in_buffer_coords.left().as_value(),
                        source_in_buffer_coords.top().as_value(),
                        source_in_buffer_coords.size.width.as_value(),
                        source_in_buffer_coords.size.height.as_value(),
                        buffer_size.width.as_uint32_t(),
                        buffer_size.height.as_uint32_t()
                    };
                }
                return source_in_buffer_coords;
            }
            else
            {
                return entire_buffer;
            }
        }();

    auto const logical_size =
        [&]()
        {
            if (destination)
            {
                return destination.value();
            }
            if (as_int_size_if_exact(src_bounds.size))
            {
                return as_int_size_if_exact(src_bounds.size).value() / scale;
            }
            else
            {
                throw wayland::ProtocolError{
                    resource,
                    wayland::Viewport::Error::bad_size,
                    "No wp_viewport destination set, and source size (%f × %f) is not integral", src_bounds.size.width.as_value(), src_bounds.size.height.as_value()};
            }
        }();

    viewport_changed = false;
    return std::make_pair(src_bounds, logical_size);
}

void mf::Viewport::set_source(double x, double y, double width, double height)
{
    if (!surface)
    {
        throw wayland::ProtocolError{
            resource,
            Error::no_surface,
            "Surface associated with viewport has been destroyed"};
    }
    // We know the parameters are coming from a 16.16 wl_fixed value, so doubles give exact representation
    if (x < 0 || y < 0 || width <= 0 || height <= 0)
    {
        // All values == -1 means “unset the source viewport”
        if ((x == y) && (y == width) && (width == height) && (height == -1.0f))
        {
            source = std::nullopt;
        }
        else
        {
            throw wayland::ProtocolError{
                resource,
                Error::bad_value,
                "Source viewport (%f, %f), (%f × %f) invalid: (x,y) must be non-negative, (width, height) must be positive",
                x, y,
                width, height
            };
        }
    }
    else
    {
        source = geometry::RectangleD{{x, y}, {width, height}};
    }
    viewport_changed = true;
}

void mf::Viewport::set_destination(int32_t width, int32_t height)
{
    if (!surface)
    {
        throw wayland::ProtocolError{
            resource,
            Error::no_surface,
            "Surface associated with viewport has been destroyed"};
    }
    if (width <= 0 || height <= 0)
    {
        if (width == -1 && height == -1)
        {
            destination = std::nullopt;
        }
        else
        {
            throw wayland::ProtocolError{
                resource,
                Error::bad_value,
                "Destination viewport (%i × %i) invalid: (width, height) must both be positive",
                width, height
            };
        }
    }
    else
    {
        destination = geometry::Size{width, height};
    }

    viewport_changed = true;
}
