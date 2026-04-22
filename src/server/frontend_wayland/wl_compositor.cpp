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

#include "wl_compositor.h"
#include "wl_surface.h"
#include "wl_region.h"

namespace mw = mir::wayland_rs;
namespace mf = mir::frontend;

mf::WlCompositor::WlCompositor(std::shared_ptr<WlClient> const& client)
{
}

auto mf::WlCompositor::create_region() -> std::shared_ptr<wayland_rs::WlRegionImpl>
{
    return std::make_shared<WlRegion>();
}

auto mf::WlCompositor::create_surface() -> std::shared_ptr<wayland_rs::WlSurfaceImpl>
{

}
