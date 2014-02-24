/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_RENDERABLE_H_
#define MIR_GRAPHICS_RENDERABLE_H_

#include <mir/geometry/rectangle.h>
#include <glm/glm.hpp>
#include <memory>

namespace mir
{
namespace graphics
{

class Buffer;
class Renderable
{
public:
    virtual std::shared_ptr<Buffer> buffer(unsigned long frameno) const = 0;
    virtual bool alpha_enabled() const = 0;
    virtual geometry::Rectangle screen_position() const = 0;

    // These are from the old CompositingCriteria. There is a little bit
    // of function overlap with the above functions still.
    virtual float alpha() const = 0;
    virtual glm::mat4 const& transformation() const = 0;
    virtual bool should_be_rendered_in(geometry::Rectangle const& rect) const = 0;
    virtual bool shaped() const = 0;  // meaning the pixel format has alpha


protected:
    Renderable() = default;
    virtual ~Renderable() = default;
    Renderable(Renderable const&) = delete;
    Renderable& operator=(Renderable const&) = delete;
};

}
}

#endif /* MIR_GRAPHICS_RENDERABLE_H_ */
