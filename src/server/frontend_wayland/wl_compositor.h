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

#ifndef MIR_FRONTEND_WL_COMPOSITOR_H
#define MIR_FRONTEND_WL_COMPOSITOR_H

#include "wayland.h"
#include "wl_client.h"

namespace mir
{
namespace frontend
{
class WlCompositor : public wayland_rs::WlCompositorImpl
{
public:
    WlCompositor(std::shared_ptr<WlClient> const& client);

    auto create_surface() -> std::shared_ptr<wayland_rs::WlSurfaceImpl> override;
    auto create_region() -> std::shared_ptr<wayland_rs::WlRegionImpl> override;

private:
    std::shared_ptr<WlClient> client;
};
}
}

#endif //MIR_FRONTEND_WL_COMPOSITOR_H
