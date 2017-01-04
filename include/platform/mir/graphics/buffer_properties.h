/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_BUFFER_PROPERTIES_H_
#define MIR_GRAPHICS_BUFFER_PROPERTIES_H_

#include "mir/geometry/size.h"
#include "mir_toolkit/common.h"

namespace mir
{
namespace graphics
{

/**
 * How a buffer is going to be used.
 *
 * The usage is not just a hint; for example, depending on the platform, a
 * 'hardware' buffer may not support direct pixel access.
 */
enum class BufferUsage
{
    undefined,
    /** rendering using GL */
    hardware,
    /** rendering using direct pixel access */
    software
};

/**
 * Buffer creation properties.
 */
struct BufferProperties
{
    BufferProperties() :
        size(),
        format(mir_pixel_format_invalid),
        usage(BufferUsage::undefined)
    {
    }

    BufferProperties(const geometry::Size& size,
                     const MirPixelFormat& format,
                     BufferUsage usage) :
        size{size},
        format{format},
        usage{usage}
    {
    }

    geometry::Size size;
    MirPixelFormat format;
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
#endif // MIR_GRAPHICS_BUFFER_PROPERTIES_H_
