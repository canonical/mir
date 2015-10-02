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
#include <mir/graphics/renderable.h>
#include <mir_toolkit/common.h>

#include <memory>

namespace mir
{
namespace graphics
{

class Buffer;

class NativeDisplayBuffer
{
protected:
    NativeDisplayBuffer() = default;
    virtual ~NativeDisplayBuffer() = default;
    NativeDisplayBuffer(NativeDisplayBuffer const&) = delete;
    NativeDisplayBuffer operator=(NativeDisplayBuffer const&) = delete;
};

/**
 * Interface to an output framebuffer.
 */
class DisplayBuffer
{
public:
    virtual ~DisplayBuffer() = default;

    /** The area the DisplayBuffer occupies in the virtual screen space. */
    virtual geometry::Rectangle view_area() const = 0;

    /** This will render renderlist to the screen and post the result to the 
     *  screen if there is a hardware optimization that can be done.
     *  \param [in] renderlist 
     *      The renderables that should appear on the screen if the hardware
     *      is capable of optmizing that list somehow. If what you want
     *      displayed on the screen cannot be represented by a RenderableList,
     *      then you should draw using OpenGL and use post_update()
     *  \returns
     *      true if the hardware can optimize the rendering of the list.
     *      When this call completes, the renderlist will have been posted
     *      to the screen.
     *      false if the hardware platform cannot optimize the list. The screen
     *      will not be updated. The caller should render the list another way,
     *      and post using post_update()
    **/
    virtual bool post_renderables_if_optimizable(RenderableList const& renderlist) = 0;

    /** Returns the orientation of the display buffer relative to how the
     *  user should see it (the orientation of the output).
     *  This tells us how much (if any) rotation the renderer needs to do.
     *  If your DisplayBuffer can do the rotation itself then this will
     *  always return mir_orientation_normal. If the DisplayBuffer does not
     *  implement the rotation itself then this function will return the
     *  amount of rotation the renderer must do to make things "look right".
     */
    virtual MirOrientation orientation() const = 0;

    /** Returns a pointer to the native display buffer object backing this
     *  display buffer.
     *
     *  The pointer to the native display buffer remains valid as long as the
     *  display buffer object is valid.
     */
    virtual NativeDisplayBuffer* native_display_buffer() = 0;

protected:
    DisplayBuffer() = default;
    DisplayBuffer(DisplayBuffer const& c) = delete;
    DisplayBuffer& operator=(DisplayBuffer const& c) = delete;
};

}
}

#endif /* MIR_GRAPHICS_DISPLAY_BUFFER_H_ */
