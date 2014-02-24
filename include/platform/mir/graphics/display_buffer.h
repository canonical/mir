/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_DISPLAY_BUFFER_H_
#define MIR_GRAPHICS_DISPLAY_BUFFER_H_

#include <mir/geometry/rectangle.h>
#include <mir_toolkit/common.h>

#include <memory>
#include <functional>
#include <list>

namespace mir
{
namespace graphics
{

class Buffer;
class Renderable;

/**
 * Interface to an output framebuffer.
 */
class DisplayBuffer
{
public:
    virtual ~DisplayBuffer() {}

    /** The area the DisplayBuffer occupies in the virtual screen space. */
    virtual geometry::Rectangle view_area() const = 0;
    /** Makes the DisplayBuffer the current GL rendering target. */
    virtual void make_current() = 0;
    /** Releases the current GL rendering target. */
    virtual void release_current() = 0;

    /** This will trigger OpenGL rendering and post the result to the screen. */
    virtual void post_update() = 0;

    /** This will render renderlist to the screen and post the result to the screen.
        For each renderable, the DisplayBuffer will decide if its more efficient to render
        that Renderable via OpenGL, or via another method. If the Renderable is to be rendered
        via OpenGL, render_fn will be invoked on that Renderable. */
    virtual void render_and_post_update(
        std::list<std::shared_ptr<Renderable>> const& renderlist,
        std::function<void(Renderable const&)> const& render_fn) = 0;

    /** to be deprecated */
    virtual bool can_bypass() const = 0;
    virtual void post_update(std::shared_ptr<Buffer> /* bypass_buf */) {}

    /** Returns the orientation of the display buffer relative to how the
     *  user should see it (the orientation of the output).
     *  This tells us how much (if any) rotation the renderer needs to do.
     *  If your DisplayBuffer can do the rotation itself then this will
     *  always return mir_orientation_normal. If the DisplayBuffer does not
     *  implement the rotation itself then this function will return the
     *  amount of rotation the renderer must do to make things "look right".
     */
    virtual MirOrientation orientation() const = 0;

protected:
    DisplayBuffer() = default;
    DisplayBuffer(DisplayBuffer const& c) = delete;
    DisplayBuffer& operator=(DisplayBuffer const& c) = delete;
};

}
}

#endif /* MIR_GRAPHICS_DISPLAY_BUFFER_H_ */
