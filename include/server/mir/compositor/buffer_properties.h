/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_COMPOSITOR_BUFFER_PROPERTIES_H_
#define MIR_COMPOSITOR_BUFFER_PROPERTIES_H_

#include "mir/geometry/size.h"
#include "mir/geometry/pixel_format.h"

namespace mir
{
namespace compositor
{

enum class BufferUsage
{
    undefined,
    hardware,
    software
};

struct BufferProperties
{
    BufferProperties() :
        size(),
        format(geometry::PixelFormat::invalid),
        usage(BufferUsage::undefined)
    {
    }

    BufferProperties(const geometry::Size& size,
                     const geometry::PixelFormat& format,
                     BufferUsage usage) :
        size{size},
        format{format},
        usage{usage}
    {
    }

    geometry::Size size;
    geometry::PixelFormat format;
    BufferUsage usage;
};

inline bool operator==(BufferProperties const& lhs, BufferProperties const& rhs)
{
    return lhs.size == rhs.size &&
           lhs.format == rhs.format &&
           lhs.usage == rhs.usage;
}

inline bool operator!=(BufferProperties const& lhs, BufferProperties const& rhs)
{
    return !(lhs == rhs);
}

}
}
#endif // MIR_COMPOSITOR_BUFFER_PROPERTIES_H_
