/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_RENDERER_SW_PIXEL_SOURCE_H_
#define MIR_RENDERER_SW_PIXEL_SOURCE_H_

#include <stddef.h>
#include <functional>
#include "mir/geometry/dimensions.h"

namespace mir
{
namespace renderer
{
namespace software
{

class PixelSource
{
public:
    virtual ~PixelSource() = default;

    //functions have legacy issues with their signatures. 
    //FIXME: correct write, it requires that the user does too much to use it correctly,
    //       (ie, it forces them to figure out what size is proper, alloc a buffer, fill it, and then
    //       copy the data into the buffer)
    virtual void write(unsigned char const* pixels, size_t size) = 0;
    //FIXME: correct read, it doesn't give size or format information about the pixels.
    virtual void read(std::function<void(unsigned char const*)> const& do_with_pixels) = 0;
    virtual geometry::Stride stride() const = 0;

protected:
    PixelSource() = default;
    PixelSource(PixelSource const&) = delete;
    PixelSource& operator=(PixelSource const&) = delete;
};

}
}
}

#endif /* MIR_RENDERER_SW_PIXEL_SOURCE_H_ */
