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

#ifndef EXAMPLE_SERVER_LIB_WAYLAND_SURFACE_H
#define EXAMPLE_SERVER_LIB_WAYLAND_SURFACE_H

#include "wayland_app.h"

#include <mir/geometry/size.h>

#include <functional>

class WaylandSurface
{
public:
    WaylandSurface(WaylandApp const* app);
    virtual ~WaylandSurface() = default;

    void attach_buffer(wl_buffer* buffer, int scale);
    void commit() const;
    /// output can be null, user needs to commit after
    void set_fullscreen(wl_output* output);
    void add_frame_callback(std::function<void()>&& func);

    auto app() const -> WaylandApp const* { return app_; }
    auto surface() const -> wl_surface* { return surface_; }
    auto configured_size() const -> mir::geometry::Size { return configured_size_; };

protected:
    /// Called when the compositor configures this shell surface
    virtual void configured() {};

private:
    static void handle_surface_configure(void* data, xdg_surface* xdg_surface, uint32_t serial);
    static void handle_toplevel_configure(
        void* data,
        xdg_toplevel* toplevel,
        int32_t width,
        int32_t height,
        wl_array* states);

    static xdg_surface_listener const xdg_surface_listener_;
    static xdg_toplevel_listener const xdg_toplevel_listener_;
    static mir::geometry::Size const default_size;

    WaylandApp const* const app_;
    WaylandObject<wl_surface> const surface_;
    WaylandObject<xdg_surface> const xdg_surface_;
    WaylandObject<xdg_toplevel> const xdg_toplevel_;
    mir::geometry::Size configured_size_;
    mir::geometry::Size pending_size_;
    int buffer_scale{1};
};


#endif // EXAMPLE_SERVER_LIB_WAYLAND_SURFACE_H
