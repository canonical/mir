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

#include "layer_shell_wayland_surface.h"

namespace geom = mir::geometry;

void miral::tk::LayerShellWaylandSurface::configure(void* data,
    zwlr_layer_surface_v1 *layer_surface_v1,
    uint32_t serial,
    uint32_t width,
    uint32_t height)
{
    auto const wayland_surface = static_cast<LayerShellWaylandSurface*>(data);
    wayland_surface->configured_size_ = mir::geometry::Size{width, height};
    zwlr_layer_surface_v1_ack_configure(layer_surface_v1, serial);
}

void miral::tk::LayerShellWaylandSurface::closed(void* data, zwlr_layer_surface_v1* zwlr_layer_surface_v1)
{
    (void)data;
    (void)zwlr_layer_surface_v1;
}

zwlr_layer_surface_v1_listener miral::tk::LayerShellWaylandSurface::layer_surface_listener = {
    configure,
    closed
};

miral::tk::LayerShellWaylandSurface::LayerShellWaylandSurface(
    WaylandApp const* app,
    CreationParams const& creation_params)
    : surface_{wl_compositor_create_surface(app->compositor()), wl_surface_destroy},
      layer_surface_{
          zwlr_layer_shell_v1_get_layer_surface(app->layer_shell(), surface_, nullptr, creation_params.layer, "miral"),
          zwlr_layer_surface_v1_destroy
      }
{
    zwlr_layer_surface_v1_set_anchor(layer_surface_, creation_params.anchor);
    zwlr_layer_surface_v1_set_size(layer_surface_, creation_params.size.width.as_value(), creation_params.size.height.as_value());
    zwlr_layer_surface_v1_add_listener(layer_surface_, &layer_surface_listener, this);
    app->roundtrip();

    attach_buffer(nullptr, 1);
    commit();
}

void miral::tk::LayerShellWaylandSurface::attach_buffer(wl_buffer* buffer, int scale)
{
    if (buffer_scale != scale)
    {
        wl_surface_set_buffer_scale(surface_, scale);
        buffer_scale = scale;
    }
    wl_surface_attach(surface_, buffer, 0, 0);
}

void miral::tk::LayerShellWaylandSurface::commit() const
{
    wl_surface_commit(surface_);
}
