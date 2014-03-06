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
    /**
     * Return the next buffer that should be composited/rendered.
     *
     * \param [in] frameno The frameno parameter is important for
     *                     multi-monitor platforms. Calls with the same frameno
     *                     will get the same buffer returned. This ensures that
     *                     a surface visible on multiple outputs does not get
     *                     its buffers consumed any faster than the refresh
     *                     rate of a single monitor. Implementations may ignore
     *                     the value of frameno on single-monitor platforms
     *                     only. The caller should always ensure the frameno
     *                     is different to the previous frame. The exact value
     *                     of frameno is not important but a large range of
     *                     values is recommended.
     */
    virtual std::shared_ptr<Buffer> buffer(unsigned long frameno) const = 0;

    virtual bool alpha_enabled() const = 0;
    virtual geometry::Rectangle screen_position() const = 0;

    // These are from the old CompositingCriteria. There is a little bit
    // of function overlap with the above functions still.
    virtual float alpha() const = 0;
    virtual glm::mat4 transformation() const = 0;
    virtual bool should_be_rendered_in(geometry::Rectangle const& rect) const = 0;
    virtual bool shaped() const = 0;  // meaning the pixel format has alpha
    virtual int buffers_ready_for_compositor() const = 0;

protected:
    Renderable() = default;
    virtual ~Renderable() = default;
    Renderable(Renderable const&) = delete;
    Renderable& operator=(Renderable const&) = delete;
};

}
}

#endif /* MIR_GRAPHICS_RENDERABLE_H_ */
