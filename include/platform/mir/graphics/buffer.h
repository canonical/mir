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

#ifndef MIR_GRAPHICS_BUFFER_H_
#define MIR_GRAPHICS_BUFFER_H_

#include "mir/graphics/native_buffer.h"
#include "mir/graphics/buffer_id.h"
#include "mir/geometry/size.h"
#include "mir_toolkit/common.h"

#include <memory>
#include <functional>

namespace mir
{
namespace graphics
{

class NativeBufferBase
{
protected:
    NativeBufferBase() = default;
    virtual ~NativeBufferBase() = default;
    NativeBufferBase(NativeBuffer const&) = delete;
    NativeBufferBase operator=(NativeBuffer const&) = delete;
};

class Buffer
{
public:
    virtual ~Buffer() {}

    virtual std::shared_ptr<NativeBuffer> native_buffer_handle() const = 0;
    virtual BufferID id() const = 0;
    virtual geometry::Size size() const = 0;
    virtual geometry::Stride stride() const = 0;
    virtual MirPixelFormat pixel_format() const = 0;
    virtual void gl_bind_to_texture() = 0;
    //FIXME: correct mg::Buffer::write, it requires that the user does too much to use it correctly,
    //       (ie, it forces them to figure out what size is proper, alloc a buffer, fill it, and then
    //       copy the data into the buffer)
    virtual void write(unsigned char const* pixels, size_t size) = 0;
    virtual void read(std::function<void(unsigned char const*)> const& do_with_pixels) = 0;

    virtual NativeBufferBase* native_buffer_base() = 0;

protected:
    Buffer() = default;
};

}
}
#endif // MIR_GRAPHICS_BUFFER_H_
