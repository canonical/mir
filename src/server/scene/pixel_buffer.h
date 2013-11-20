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
 * Authored By: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_SCENE_PIXEL_BUFFER_H_
#define MIR_SCENE_PIXEL_BUFFER_H_

#include "mir/geometry/size.h"
#include "mir/geometry/dimensions.h"

namespace mir
{
namespace graphics
{
class Buffer;
}
namespace scene
{
/**
 * Interface for extracting the pixels from a graphics::Buffer.
 */
class PixelBuffer
{
public:
    virtual ~PixelBuffer() = default;

    /**
     * Fills the PixelBuffer with the contents of a graphics::Buffer.
     *
     * \param [in] buffer the buffer to get the pixels of
     */
    virtual void fill_from(graphics::Buffer& buffer) = 0;

    /**
     * The pixels in 0xAARRGGBB format.
     *
     * The pixel data is owned by the PixelBuffer object and is only valid
     * until the next call to fill_from().
     *
     * This method may involve transformation of the extracted data.
     */
    virtual void const* as_argb_8888() = 0;

    /**
     * The size of the pixel buffer.
     */
    virtual geometry::Size size() const = 0;

    /**
     * The stride of the pixel buffer.
     */
    virtual geometry::Stride stride() const = 0;

protected:
    PixelBuffer() = default;
    PixelBuffer(PixelBuffer const&) = delete;
    PixelBuffer& operator=(PixelBuffer const&) = delete;
};

}
}

#endif /* MIR_SCENE_PIXEL_BUFFER_H_ */
