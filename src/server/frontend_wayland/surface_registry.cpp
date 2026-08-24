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
#include "wl_surface.h"

#include "weak.h"

namespace mf = mir::frontend;
namespace mwrs = mir::wayland;

void mf::SurfaceRegistry::add_surface(
    std::shared_ptr<input::Surface const> const& surf, mwrs::Weak<mf::WlSurface> const& wl_surf)
{
    scene_surface_to_wayland_surface.insert({surf, wl_surf});

    if (wl_surf)
    {
        auto& surface = wl_surf.value();
        ObjectKey const key{surface.session.get(), surface.object_id()};
        wayland_surface_by_object[key] = wl_surf;
        object_key_by_scene_surface[surf.get()] = key;
    }
}

void mf::SurfaceRegistry::remove_surface(std::shared_ptr<input::Surface const> const& surf)
{
    scene_surface_to_wayland_surface.erase(surf);

    if (auto const iter = object_key_by_scene_surface.find(surf.get());
        iter != object_key_by_scene_surface.end())
    {
        wayland_surface_by_object.erase(iter->second);
        object_key_by_scene_surface.erase(iter);
    }
}

auto mf::SurfaceRegistry::lookup_wayland_surface(std::shared_ptr<input::Surface const> const& surf)
    -> std::optional<mwrs::Weak<mf::WlSurface>>
{
    if (auto const iter = scene_surface_to_wayland_surface.find(surf); iter != scene_surface_to_wayland_surface.end())
    {
        return iter->second;
    }

    return std::nullopt;
}

auto mf::SurfaceRegistry::scene_surface_for(scene::Session const& session, uint32_t id) const
    -> std::shared_ptr<scene::Surface>
{
    if (auto const iter = wayland_surface_by_object.find({&session, id});
        iter != wayland_surface_by_object.end())
    {
        if (auto const& wl_surf = iter->second)
            return wl_surf.value().scene_surface().value_or(nullptr);
    }

    return nullptr;
}
