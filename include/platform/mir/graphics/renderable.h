/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_GRAPHICS_RENDERABLE_H_
#define MIR_GRAPHICS_RENDERABLE_H_

#include <optional>
#include <mir/geometry/rectangle.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "mir_toolkit/common.h"

namespace mir
{

namespace scene { class Surface; }

namespace graphics
{

class Buffer;
class Renderable
{
public:
    virtual ~Renderable() = default;

    typedef void const* ID; // Mostly opaque, but zero is reserved as "invalid"

    /**
     * Return a unique ID for the renderable, which may or may not be based
     * on the underlying surface ID. You should not assume they are related.
     */
    virtual ID id() const = 0;

    /**
     * Return the buffer that should be composited/rendered.
     */
    virtual std::shared_ptr<Buffer> buffer() const = 0;

    virtual geometry::Rectangle screen_position() const = 0;
    /**
     * The region of \ref buffer that should be sampled from.
     *
     * This is in buffer coordinates, so {{0, 0}, {buffer->size().width, buffer->size().height} means
     * “use the whole buffer”.
     */
    virtual geometry::RectangleD src_bounds() const = 0;
    virtual std::optional<geometry::Rectangle> clip_area() const = 0;

    // These are from the old CompositingCriteria. There is a little bit
    // of function overlap with the above functions still.
    virtual float alpha() const = 0;

    /**
     * Transformation returns the transformation matrix that should be applied
     * to the surface. By default when there are no transformations this will
     * be the identity matrix.
     *
     * \warning As this functionality is presently only used by
     *          mir_demo_standalone_render_surfaces for rotations it may be
     *          deprecated in future. It is expected that real transformations
     *          may become more transient things (e.g. applied by animation
     *          logic externally instead of being a semi-permanent attribute of
     *          the surface itself).
     */
    virtual glm::mat4 transformation() const = 0;

    /**
     * The orientation of the buffer.
     */
    virtual MirOrientation orientation() const = 0;

    virtual bool shaped() const = 0;  // meaning the pixel format has alpha

    virtual auto surface_if_any() const
        -> std::optional<mir::scene::Surface const*> = 0;
protected:
    Renderable() = default;
    Renderable(Renderable const&) = delete;
    Renderable& operator=(Renderable const&) = delete;
};

typedef std::vector<std::shared_ptr<Renderable>> RenderableList;

}
}

#endif /* MIR_GRAPHICS_RENDERABLE_H_ */
