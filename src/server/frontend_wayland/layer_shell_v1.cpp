/*
 * Copyright © 2018 Canonical Ltd.
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "layer_shell_v1.h"

#include "wl_surface.h"
#include "window_wl_surface_role.h"

namespace mf = mir::frontend;
namespace geom = mir::geometry;

namespace mir
{
namespace frontend
{

LayerShellV1::LayerShellV1(struct wl_display* display, std::shared_ptr<Shell> const shell, WlSeat& seat,
                           OutputManager* output_manager)
    : wayland::LayerShellV1(display, 1),
      shell{shell},
      seat{seat},
      output_manager{output_manager}
{
}

void LayerShellV1::get_layer_surface(struct wl_client* client, struct wl_resource* resource, uint32_t id,
                                     struct wl_resource* surface, std::experimental::optional<struct
                                     wl_resource*> const& output, uint32_t layer, std::string const& namespace_)
{
    (void)client;
    (void)resource;
    (void)id;
    (void)surface;
    (void)output;
    (void)layer;
    (void)namespace_;
}

}
}
