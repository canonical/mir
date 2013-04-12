/*
 * Copyright © 2012 Canonical Ltd.
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

#ifndef MIR_GRAPHICS_RENDERABLE_H_
#define MIR_GRAPHICS_RENDERABLE_H_

#include "mir/geometry/point.h"
#include "mir/geometry/size.h"
#include <memory>

// Unfortunately we have to ignore warnings/errors in 3rd party code.
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wall"
#include <glm/glm.hpp>
#pragma GCC diagnostic pop

namespace mir
{
namespace surfaces
{
class GraphicRegion;
}
namespace graphics
{

// The interface by which a Renderer talks to, e.g., a surface.
class Renderable
{
public:
    virtual ~Renderable() {}

    virtual geometry::Point top_left() const = 0;
    virtual geometry::Size size() const = 0;
    virtual std::shared_ptr<surfaces::GraphicRegion> graphic_region() const = 0;
    virtual glm::mat4 transformation() const = 0;
    virtual float alpha() const = 0;
    virtual bool hidden() const = 0;

protected:
    Renderable() = default;
    Renderable(const Renderable&) = delete;
    Renderable& operator=(const Renderable&) = delete;
};

}
}

#endif // MIR_GRAPHICS_RENDERABLE_H_
