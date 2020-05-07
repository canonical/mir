/*
 * Copyright © 2019 Canonical Ltd.
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
 *
 * Authored by: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_FRONTEND_SURFACE_STACK_H_
#define MIR_FRONTEND_SURFACE_STACK_H_

#include <memory>
#include <set>
#include <vector>

namespace mir
{
namespace scene
{
class Observer;
class Surface;
using SurfaceSet = std::set<std::weak_ptr<scene::Surface>, std::owner_less<std::weak_ptr<scene::Surface>>>;
using SurfaceList = std::vector<std::weak_ptr<scene::Surface>>;
}
namespace frontend
{
class SurfaceStack
{
public:
    SurfaceStack() = default;
    virtual ~SurfaceStack() = default;

    virtual void add_observer(std::shared_ptr<scene::Observer> const& observer) = 0;
    virtual void remove_observer(std::weak_ptr<scene::Observer> const& observer) = 0;

    /// Returns the stacking order of the given surfaces, bottom to top
    /// If a surface is not known to the surface stack it will not be in the returned list
    virtual auto stacking_order_of(scene::SurfaceSet const& surfaces) const -> scene::SurfaceList = 0;

private:
    SurfaceStack(SurfaceStack const&) = delete;
    SurfaceStack& operator=(SurfaceStack const&) = delete;
};
}
} // namespace mir

#endif // MIR_FRONTEND_SURFACE_STACK_H_
