/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Authored By: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_test_framework/visible_surface.h"
namespace mtf = mir_test_framework;

mtf::VisibleSurface::VisibleSurface(MirSurfaceSpec* spec) :
    visible{false}
{
    mir_surface_spec_set_event_handler(spec, VisibleSurface::event_callback, this);
    surface = mir_surface_create_sync(spec);
    // Swap buffers to ensure surface is visible for event based tests
    if (mir_surface_is_valid(surface))
    {
        mir_buffer_stream_swap_buffers_sync(mir_surface_get_buffer_stream(surface));

        std::unique_lock<std::mutex> lk(mutex);
        if (!cv.wait_for(lk, std::chrono::seconds(5), [this] { return visible; }))
            throw std::runtime_error("timeout waiting for visibility of surface");
    }
}

mtf::VisibleSurface::~VisibleSurface()
{
    if (surface) mir_surface_release_sync(surface);
}

void mtf::VisibleSurface::event_callback(MirSurface* surf, MirEvent const* ev, void* context)
{
    if (mir_event_get_type(ev) == mir_event_type_surface)
    {
        if (mir_surface_event_get_attribute(mir_event_get_surface_event(ev)) == mir_surface_attrib_visibility)
        {
            auto ctx = reinterpret_cast<VisibleSurface*>(context);
            ctx->set_visibility(surf, mir_surface_event_get_attribute_value(mir_event_get_surface_event(ev)));
        }
    }
}

void mtf::VisibleSurface::set_visibility(MirSurface* surf, bool vis)
{
    std::lock_guard<std::mutex> lk(mutex);
    if (surf != surface) return;
    visible = vis;
    cv.notify_all();
}

mtf::VisibleSurface::operator MirSurface*() const
{
    return surface;
}

mtf::VisibleSurface::VisibleSurface(VisibleSurface&& that) :
    surface{that.surface}
{
    that.surface = nullptr;
}

mtf::VisibleSurface& mtf::VisibleSurface::operator=(VisibleSurface&& that)
{
    std::swap(that.surface, surface);
    return *this;
}
