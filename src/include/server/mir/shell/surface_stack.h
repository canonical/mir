/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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


#ifndef MIR_SHELL_SURFACE_COORDINATOR_H_
#define MIR_SHELL_SURFACE_COORDINATOR_H_

#include "mir/geometry/forward.h"

#include <memory>
#include <set>

namespace mir
{
namespace input { enum class InputReceptionMode; }
namespace scene
{
class Surface;
class SurfaceObserver;
class Session;
using SurfaceSet = std::set<std::weak_ptr<scene::Surface>, std::owner_less<std::weak_ptr<scene::Surface>>>;
}

namespace shell
{
class SurfaceStack
{
public:
    virtual void add_surface(
        std::shared_ptr<scene::Surface> const&,
        input::InputReceptionMode new_mode) = 0;

    virtual void raise(std::weak_ptr<scene::Surface> const& surface) = 0;

    virtual void raise(scene::SurfaceSet const& surfaces) = 0;

    virtual void remove_surface(std::weak_ptr<scene::Surface> const& surface) = 0;

    virtual auto surface_at(geometry::Point) const -> std::shared_ptr<scene::Surface> = 0;

    virtual void swap_z_order(scene::SurfaceSet const& first, scene::SurfaceSet const& second) = 0;

    virtual void send_to_back(scene::SurfaceSet const& surfaces) = 0;

    virtual auto is_above(std::weak_ptr<scene::Surface> const& a, std::weak_ptr<scene::Surface> const& b) const
        -> bool = 0;

protected:
    SurfaceStack() = default;
    virtual ~SurfaceStack() = default;
    SurfaceStack(SurfaceStack const&) = delete;
    SurfaceStack& operator=(SurfaceStack const&) = delete;
};
}
}


#endif /* MIR_SHELL_SURFACE_COORDINATOR_H_ */
