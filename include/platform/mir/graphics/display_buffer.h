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

#include "mir/graphics/platform.h"
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

    /** The size in pixels of the underlying display */
    virtual auto pixel_size() const -> geometry::Size = 0;

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

    /**
     * Attempt to acquire a platform-specific provider from this DisplayBuffer
     *
     * Any given platform is not guaranteed to implement any specific interface,
     * and the set of supported interfaces may depend on the runtime environment.
     *
     * Since this may result in a runtime probe the call may be costly, and the
     * result should be saved rather than re-acquiring an interface each time.
     *
     * \tparam Allocator
     * \return  On success: a non-null pointer to an Allocator implementation.
     *                      The lifetime of this Allocator implementation is bound
     *                      to that of the parent DisplayBuffer.
     *          On failure: nullptr
     */
    template<typename Allocator>
    auto acquire_compatible_allocator() -> Allocator*
    {
        static_assert(
            std::is_convertible_v<Allocator*, DisplayAllocator*>,
            "Can only acquire a DisplayAllocator; Provider must implement DisplayAllocator");

        if (auto const base_interface = maybe_create_allocator(typename Allocator::Tag{}))
        {
            if (auto const requested_interface = dynamic_cast<Allocator*>(base_interface))
            {
                return requested_interface;
            }
            BOOST_THROW_EXCEPTION((std::logic_error{
                "Implementation error! Platform returned object that does not support requested interface"}));
        }
        return nullptr;
    }

protected:
    /**
     * Acquire a specific hardware interface
     *
     * This should perform any runtime checks necessary to verify the requested interface is
     * expected to work and return a pointer to an implementation of that interface.
     *
     * \param type_tag  [in]    An instance of the Tag type for the requested interface.
     *                          Implementations are expected to dynamic_cast<> this to
     *                          discover the specific interface being requested.
     * \return      A pointer to an implementation of the DisplayAllocator-derived
     *              interface that corresponds to the most-derived type of tag_type.
     */
    virtual auto maybe_create_allocator(DisplayAllocator::Tag const& type_tag)
        -> DisplayAllocator* = 0;

    DisplayBuffer() = default;
    DisplayBuffer(DisplayBuffer const& c) = delete;
    DisplayBuffer& operator=(DisplayBuffer const& c) = delete;
};

}
}

#endif /* MIR_GRAPHICS_DISPLAY_BUFFER_H_ */
