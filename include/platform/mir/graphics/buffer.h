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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_COMPOSITOR_BUFFER_H_
#define MIR_COMPOSITOR_BUFFER_H_

#include "mir_toolkit/mir_native_buffer.h"
#include "mir/geometry/size.h"
#include "mir/geometry/pixel_format.h"

#include <memory>

namespace mir
{
namespace graphics
{
class BufferID;

class Buffer
{
public:
    virtual ~Buffer() {}

    virtual std::shared_ptr<MirNativeBuffer> native_buffer_handle() const = 0;
    virtual BufferID id() const = 0;
    virtual geometry::Size size() const = 0;
    virtual geometry::Stride stride() const = 0;
    virtual geometry::PixelFormat pixel_format() const = 0;
    virtual void bind_to_texture() = 0;

protected:
    Buffer() = default;
};

}
}
#endif // MIR_COMPOSITOR_BUFFER_H_
