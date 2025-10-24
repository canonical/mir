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

#include <memory>
#include <optional>
#include <unordered_map>

namespace mir
{
namespace input
{
class Surface;
}
namespace wayland
{
template <typename T> class Weak;
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

private:
    std::unordered_map<std::shared_ptr<input::Surface const>, wayland::Weak<frontend::WlSurface> const>
        scene_surface_to_wayland_surface;
};

}
}

#endif
