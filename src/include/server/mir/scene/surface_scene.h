/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_SCENE_SURFACE_SCENE_H_
#define MIR_SCENE_SURFACE_SCENE_H_

#include <memory>

namespace mir
{
namespace compositor
{
class Scene;
}
namespace scene
{
class Surface;
auto create_surface_scene(std::weak_ptr<Surface> const& surface) -> std::shared_ptr<compositor::Scene>;
}
}

#endif /* MIR_SCENE_SURFACE_SCENE_H_ */
