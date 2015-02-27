/*
 * Copyright © 2012-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_SCENE_SURFACE_STACK_MODEL_H_
#define MIR_SCENE_SURFACE_STACK_MODEL_H_

#include "mir/frontend/surface_id.h"
#include "mir/scene/depth_id.h"
#include "mir/input/input_reception_mode.h"

#include <memory>
#include <set>

namespace mir
{
namespace geometry { class Point; }

namespace scene
{
class Surface;

class SurfaceStackModel
{
public:
    using SurfaceSet = std::set<std::weak_ptr<Surface>, std::owner_less<std::weak_ptr<Surface>>>;

    virtual ~SurfaceStackModel() = default;

    virtual void add_surface(
        std::shared_ptr<Surface> const& surface,
        DepthId depth,
        input::InputReceptionMode input_mode) = 0;

    virtual void remove_surface(std::weak_ptr<Surface> const& surface) = 0;

    virtual void raise(std::weak_ptr<Surface> const& surface) = 0;

    virtual void raise(SurfaceSet const& surfaces) = 0;

    virtual auto surface_at(geometry::Point) const -> std::shared_ptr<Surface> = 0;

protected:
    SurfaceStackModel() = default;
    SurfaceStackModel(const SurfaceStackModel&) = delete;
    SurfaceStackModel& operator=(const SurfaceStackModel&) = delete;
};

}
}

#endif // MIR_SCENE_SURFACE_STACK_MODEL_H_
