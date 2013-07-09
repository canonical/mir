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

#ifndef MIR_GRAPHICS_SURFACE_INFO_H_
#define MIR_GRAPHICS_SURFACE_INFO_H_

#include "mir/geometry/rectangle.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wall"
#include <glm/glm.hpp>
#pragma GCC diagnostic pop


namespace mir
{
namespace graphics
{

class SurfaceInfo 
{
public:
    virtual float alpha() const = 0;
    virtual glm::mat4 const& transformation() const = 0;
    virtual bool should_be_rendered() const = 0;

    virtual ~SurfaceInfo() = default;

protected:
    SurfaceInfo() = default;
    SurfaceInfo(SurfaceInfo const&) = delete;
    SurfaceInfo& operator=(SurfaceInfo const&) = delete;
};

class SurfaceInfoController : public SurfaceInfo
{
public:
    virtual void set_hidden(bool hidden) = 0;
    virtual void apply_alpha(float alpha) = 0;
    virtual void apply_rotation(float degrees, glm::vec3 const&) = 0;

    virtual ~SurfaceInfoController() = default;

protected:
    SurfaceInfoController() = default;
    SurfaceInfoController(SurfaceInfoController const&) = delete;
    SurfaceInfoController& operator=(SurfaceInfoController const&) = delete;
};

}
}

#endif /* MIR_GRAPHICS_SURFACE_INFO_H_ */
