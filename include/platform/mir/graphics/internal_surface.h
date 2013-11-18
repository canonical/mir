/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_GRAPHICS_INTERNAL_SURFACE_H_
#define MIR_GRAPHICS_INTERNAL_SURFACE_H_

#include "mir/geometry/size.h"
#include "mir_toolkit/client_types.h"

#include <memory>

namespace mir
{
namespace graphics
{
class Buffer;

class InternalSurface
{
public:
    virtual ~InternalSurface() = default;

    virtual void swap_buffers(std::shared_ptr<graphics::Buffer>&) = 0;
    virtual geometry::Size size() const = 0;
    virtual MirPixelFormat pixel_format() const = 0;

protected:
    InternalSurface() = default;
    InternalSurface(InternalSurface const&) = delete;
    InternalSurface& operator=(InternalSurface const&) = delete;
};
}
}

#endif /* MIR_GRAPHICS_INTERNAL_SURFACE_H_ */
