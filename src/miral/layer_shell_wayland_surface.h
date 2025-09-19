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

#ifndef MIRAL_WAYLAND_SURFACE_H
#define MIRAL_WAYLAND_SURFACE_H

#include "wayland_app.h"

#include "mir/geometry/size.h"

#include <functional>

namespace miral::tk
{

/// A wayland surface built on wlr layer shell v1.
class LayerShellWaylandSurface
{
public:
    struct CreationParams
    {
        mir::geometry::Size size;
        uint32_t layer;
        uint32_t anchor;
    };

    /// Construct a layer shell surface for the given application.
    ///
    /// \param app the application that the surface is being created for.
    /// \param creation_params parameters that define the surface
    explicit LayerShellWaylandSurface(WaylandApp const* app, CreationParams const& creation_params);
    virtual ~LayerShellWaylandSurface() = default;

    /// Attach a buffer to the surface at the provided scale.
    ///
    /// \param buffer to attach
    /// \param scale scale of the buffer
    void attach_buffer(wl_buffer* buffer, int scale);

    /// Commit the surface.
    ///
    /// This typically follows a call to #attach_buffer.
    void commit() const;

    /// Return the configured size of the surface.
    ///
    /// \returns the size of the surface
    auto configured_size() const -> mir::geometry::Size { return configured_size_; };

private:
    static void configure(void* data,
        zwlr_layer_surface_v1 *layer_surface_v1,
        uint32_t serial,
        uint32_t width,
        uint32_t height);
    static void closed(void* data, zwlr_layer_surface_v1* zwlr_layer_surface_v1);

    static zwlr_layer_surface_v1_listener layer_surface_listener;
    WaylandObject<wl_surface> const surface_;
    WaylandObject<zwlr_layer_surface_v1> const layer_surface_;
    mir::geometry::Size configured_size_;
    int buffer_scale{1};
};
}

#endif // MIRAL_WAYLAND_SURFACE_H
