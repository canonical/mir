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

#ifndef MIR_FRONTEND_WAYLAND_SURFACE_REGISTRY_H_
#define MIR_FRONTEND_WAYLAND_SURFACE_REGISTRY_H_

#include "weak.h"

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>

namespace mir
{
namespace scene
{
class Session;
class Surface;
}
namespace input
{
class Surface;
}
namespace frontend
{
class WlSurface;

class SurfaceRegistry
{
public:
    void add_surface(
        std::shared_ptr<input::Surface const> const& surf, wayland::Weak<frontend::WlSurface> const& wl_surf);
    void remove_surface(std::shared_ptr<input::Surface const> const& surf);
    auto lookup_wayland_surface(std::shared_ptr<input::Surface const> const& surf)
        -> std::optional<wayland::Weak<frontend::WlSurface>>;

    /// Resolve the `scene::Surface` for the Wayland surface identified by
    /// (`session`, protocol object `id`), or nullptr if unknown. Intended for
    /// test/integration tooling that needs to map a client-side surface to its
    /// Mir scene surface.
    auto scene_surface_for(scene::Session const& session, uint32_t id) const
        -> std::shared_ptr<scene::Surface>;

private:
    std::unordered_map<std::shared_ptr<input::Surface const>, wayland::Weak<frontend::WlSurface> const>
        scene_surface_to_wayland_surface;

    using ObjectKey = std::pair<scene::Session const*, uint32_t>;
    std::map<ObjectKey, wayland::Weak<frontend::WlSurface>> wayland_surface_by_object;
    std::unordered_map<input::Surface const*, ObjectKey> object_key_by_scene_surface;
};

}
}

#endif
