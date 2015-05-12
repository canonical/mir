/*
 * Copyright © 2013-14 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_SCENE_SURFACE_COORDINATOR_H_
#define MIR_SCENE_SURFACE_COORDINATOR_H_

#include "mir/scene/depth_id.h"
#include <memory>
#include <set>

namespace mir
{
namespace geometry { class Point; }
namespace input { enum class InputReceptionMode; }
namespace scene
{
class Surface;
struct SurfaceCreationParameters;
class SurfaceObserver;
class Session;

class SurfaceCoordinator
{
public:
    using SurfaceSet = std::set<std::weak_ptr<scene::Surface>, std::owner_less<std::weak_ptr<scene::Surface>>>;

    virtual void add_surface(
        std::shared_ptr<Surface> const&,
        scene::DepthId depth,
        input::InputReceptionMode const& new_mode,
        Session* session) = 0;

    virtual void raise(std::weak_ptr<Surface> const& surface) = 0;

    virtual void raise(SurfaceSet const& surfaces) = 0;

    virtual void remove_surface(std::weak_ptr<Surface> const& surface) = 0;

    virtual auto surface_at(geometry::Point) const -> std::shared_ptr<Surface> = 0;

protected:
    SurfaceCoordinator() = default;
    virtual ~SurfaceCoordinator() = default;
    SurfaceCoordinator(SurfaceCoordinator const&) = delete;
    SurfaceCoordinator& operator=(SurfaceCoordinator const&) = delete;
};
}
}


#endif /* MIR_SCENE_SURFACE_COORDINATOR_H_ */
