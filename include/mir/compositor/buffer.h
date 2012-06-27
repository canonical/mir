/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_COMPOSITOR_BUFFER_H_
#define MIR_COMPOSITOR_BUFFER_H_

#include <cstdint>

namespace mir
{
namespace compositor
{

enum class PixelFormat {
    rgba_8888
};

class Buffer
{
 public:

    Buffer() : width(0),
               height(0),
               stride(0),
               pixel_format(PixelFormat::rgba_8888)
    {
    }
    
    virtual uint32_t get_width() const
    {
        return width;
    }

    virtual uint32_t get_height() const
    {
        return height;
    }

    virtual uint32_t get_stride() const
    {
        return stride;
    }

    virtual PixelFormat get_pixel_format() const
    {
        return pixel_format;
    }

  protected:
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    PixelFormat pixel_format;
    
};

}
}
#endif // MIR_COMPOSITOR_BUFFER_H_
