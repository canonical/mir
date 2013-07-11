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

#ifndef MIR_SURFACES_SURFACE_INFO_H_
#define MIR_SURFACES_SURFACE_INFO_H_

#include "mir/geometry/rectangle.h"
#include "mir/geometry/size.h"

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>

namespace mir
{
namespace surfaces
{
class SurfaceInfo
{
public:
    virtual std::string const& name() const = 0;

    /* TODO: consolidate representation of transformation with size/position? */
    virtual geometry::Rectangle size_and_position() const = 0;
    virtual glm::mat4 const& transformation() const = 0;

    virtual void apply_rotation(float degrees, glm::vec3 const&) = 0;
protected:
    SurfaceInfo() = default; 
    virtual ~SurfaceInfo() = default;
    SurfaceInfo(const SurfaceInfo&) = delete;
    SurfaceInfo& operator=(const SurfaceInfo& ) = delete;
};

class SurfaceStateModifier : public SurfaceInfo
{
public:
    virtual void move_to(geometry::Point) = 0;

protected:
    SurfaceStateModifier() = default; 
    virtual ~SurfaceStateModifier() noexcept = default;
    SurfaceStateModifier(const SurfaceStateModifier&) = delete;
    SurfaceStateModifier& operator=(const SurfaceStateModifier& ) = delete;
};

}
}
#endif /* MIR_SURFACES_SURFACE_INFO_H_ */
