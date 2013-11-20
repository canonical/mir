/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_SCENE_SURFACE_INFO_H_
#define MIR_SCENE_SURFACE_INFO_H_

#include "mir/geometry/rectangle.h"

#include <vector>

namespace mir
{
namespace scene
{

class MutableSurfaceState
{
public:
    virtual void move_to(geometry::Point) = 0;
    virtual void resize(geometry::Size const& size) = 0;
    virtual void frame_posted() = 0;
    virtual void set_hidden(bool hidden) = 0;
    virtual void apply_alpha(float alpha) = 0;
    virtual void apply_rotation(float degrees, glm::vec3 const&) = 0;
    virtual void set_input_region(
        std::vector<geometry::Rectangle> const& input_rectangles) = 0;

protected:
    MutableSurfaceState() = default; 
    virtual ~MutableSurfaceState() noexcept = default;
    MutableSurfaceState(const MutableSurfaceState&) = delete;
    MutableSurfaceState& operator=(const MutableSurfaceState& ) = delete;
};

}
}
#endif /* MIR_SCENE_SURFACE_INFO_H_ */
