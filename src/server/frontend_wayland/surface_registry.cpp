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

#include "surface_registry.h"

#include <mir/wayland/weak.h>

namespace mf = mir::frontend;
namespace mw = mir::wayland;

void mf::SurfaceRegistry::add_surface(
    std::shared_ptr<input::Surface const> const& surf, mw::Weak<mf::WlSurface> const& wl_surf)
{
    scene_surface_to_wayland_surface.insert({surf, wl_surf});
}

void mf::SurfaceRegistry::remove_surface(std::shared_ptr<input::Surface const> const& surf)
{
    scene_surface_to_wayland_surface.erase(surf);
}

auto mf::SurfaceRegistry::lookup_wayland_surface(std::shared_ptr<input::Surface const> const& surf)
    -> std::optional<mw::Weak<mf::WlSurface>>
{
    if (auto const iter = scene_surface_to_wayland_surface.find(surf); iter != scene_surface_to_wayland_surface.end())
    {
        return iter->second;
    }

    return std::nullopt;
}
