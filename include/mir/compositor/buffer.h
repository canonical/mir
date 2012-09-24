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

#include "mir/geometry/size.h"
#include "mir/compositor/pixel_format.h"

#include <memory>

namespace mir
{

namespace graphics
{
class Texture;
}

namespace compositor
{
class BufferIPCPackage;
class Buffer
{
public:

    virtual ~Buffer() {}

    virtual geometry::Size size() const = 0;

    virtual geometry::Stride stride() const = 0;

    virtual PixelFormat pixel_format() const = 0;

    virtual std::shared_ptr<BufferIPCPackage> get_ipc_package() const = 0;

    virtual void bind_to_texture() = 0;

protected:
    Buffer() = default;
    Buffer(Buffer const&) = delete;
    Buffer& operator=(Buffer const&) = delete;
};

}
}
#endif // MIR_COMPOSITOR_BUFFER_H_
