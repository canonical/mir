/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "wayland_surface.h"

namespace geom = mir::geometry;

// If building against newer Wayland protocol definitions we may miss trailing fields
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
xdg_surface_listener const WaylandSurface::xdg_surface_listener_ {
    handle_surface_configure,
};

xdg_toplevel_listener const WaylandSurface::xdg_toplevel_listener_ {
    handle_toplevel_configure,
    [](auto...){},  // close
};
#pragma GCC diagnostic pop

mir::geometry::Size const WaylandSurface::default_size{640, 480};

WaylandSurface::WaylandSurface(WaylandApp const* app)
    : app_{app},
      surface_{wl_compositor_create_surface(app->compositor()), wl_surface_destroy},
      xdg_surface_{xdg_wm_base_get_xdg_surface(app->xdg_wm_base(), surface_), xdg_surface_destroy},
      xdg_toplevel_{xdg_surface_get_toplevel(xdg_surface_), xdg_toplevel_destroy},
      configured_size_{default_size},
      pending_size_{default_size}
{
    xdg_surface_add_listener(xdg_surface_, &xdg_surface_listener_, this);
    xdg_toplevel_add_listener(xdg_toplevel_, &xdg_toplevel_listener_, this);
    // Callers must call set_fullscreen() (or other configuration) and then
    // commit() + roundtrip() to trigger the initial xdg_surface.configure event.
}

void WaylandSurface::attach_buffer(wl_buffer* buffer, int scale)
{
    if (buffer_scale != scale)
    {
        wl_surface_set_buffer_scale(surface_, scale);
        buffer_scale = scale;
    }
    wl_surface_attach(surface_, buffer, 0, 0);
    if (wl_surface_get_version(surface_) >= WL_SURFACE_DAMAGE_BUFFER_SINCE_VERSION)
        wl_surface_damage_buffer(surface_, 0, 0, INT32_MAX, INT32_MAX);
    else
        wl_surface_damage(surface_, 0, 0, INT32_MAX, INT32_MAX);
}

void WaylandSurface::commit() const
{
    wl_surface_commit(surface_);
}

void WaylandSurface::set_fullscreen(wl_output* output)
{
    xdg_toplevel_set_fullscreen(xdg_toplevel_, output);
}

void WaylandSurface::add_frame_callback(std::function<void()>&& func)
{
    WaylandCallback::create(wl_surface_frame(surface_), std::move(func));
}

void WaylandSurface::handle_surface_configure(void* data, xdg_surface* xdg_surface, uint32_t serial)
{
    auto const self = static_cast<WaylandSurface*>(data);
    xdg_surface_ack_configure(xdg_surface, serial);

    bool changed{false};

    if (self->pending_size_.width > geom::Width{} &&
        self->pending_size_.width != self->configured_size_.width)
    {
        self->configured_size_.width = self->pending_size_.width;
        changed = true;
    }

    if (self->pending_size_.height > geom::Height{} &&
        self->pending_size_.height != self->configured_size_.height)
    {
        self->configured_size_.height = self->pending_size_.height;
        changed = true;
    }

    if (changed)
    {
        self->configured();
    }
}

void WaylandSurface::handle_toplevel_configure(
    void* data,
    xdg_toplevel* /*toplevel*/,
    int32_t width,
    int32_t height,
    wl_array* /*states*/)
{
    auto const self = static_cast<WaylandSurface*>(data);

    if (width > 0)
        self->pending_size_.width = geom::Width{width};

    if (height > 0)
        self->pending_size_.height = geom::Height{height};
}
