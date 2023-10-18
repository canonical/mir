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

#ifndef MIR_GRAPHICS_DISPLAY_BUFFER_H_
#define MIR_GRAPHICS_DISPLAY_BUFFER_H_

#include <mir/geometry/rectangle.h>
#include <mir/graphics/renderable.h>
#include <mir_toolkit/common.h>
#include <glm/glm.hpp>

#include <memory>

namespace mir
{
namespace graphics
{

class Framebuffer;
class DisplayInterfaceProvider;

struct DisplayElement
{
    /// Position and size of this element on-screen
    geometry::Rectangle screen_positon;
    /** Position and size of region of the buffer to sample from
     *
     * This will typically be (0,0) ->(buffer width, buffer height)
     * to use the whole buffer (with screen_position.size = (buffer width, buffer height)).
     *
     * Scaling can be specified by having screen_position.size != source_position.size,
     * clipping can be specified by having source_position.size != buffer.size
     */
    geometry::RectangleF source_position;
    std::shared_ptr<Framebuffer> buffer;
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
     *      then you should render using a graphics library like OpenGL.
     *  \returns
     *      True if the hardware can (and has) fully composite/overlay the list;
     *      False if the hardware platform cannot composite the list, and the
     *      caller should then render the list another way using a graphics
     *      library such as OpenGL.
    **/
    virtual bool overlay(std::vector<DisplayElement> const& renderlist) = 0;

    /**
     * Set the content for the next submission of this display
     *
     * This is basically a specialisation of overlay(), above, with extra constraints
     * and guarantees. Namely:
     * * The Framebuffer must be exactly view_area().size big, and
     * * The DisplayPlatform guarantees that this call will succeed with a framebuffer
     *   allocated for this DisplayBuffer
     *
     * \param content
     */
    virtual void set_next_image(std::unique_ptr<Framebuffer> content) = 0;

    /**
     * Returns a transformation that the renderer must apply to all rendering.
     * There is usually no transformation required (just the identity matrix)
     * but in other cases this will represent transformations that the display
     * hardware is unable to do itself, such as screen rotation, flipping,
     * reflection, scaling or keystone correction.
     */
    virtual glm::mat2 transformation() const = 0;

    virtual auto display_provider() const -> std::shared_ptr<DisplayInterfaceProvider> = 0;
protected:
    DisplayBuffer() = default;
    DisplayBuffer(DisplayBuffer const& c) = delete;
    DisplayBuffer& operator=(DisplayBuffer const& c) = delete;
};

}
}

#endif /* MIR_GRAPHICS_DISPLAY_BUFFER_H_ */
