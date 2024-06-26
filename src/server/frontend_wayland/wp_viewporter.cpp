
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

namespace
{
class ViewporterInstance : public mir::wayland::Viewporter
{
public:
    ViewporterInstance(wl_resource* resource)
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

auto mf::Viewport::dirty() -> bool
{
    return std::exchange(dirty_, false);
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
    dirty_ = true;
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

    dirty_ = true;
}
