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
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_CLIENT_BUFFER_H_
#define MIR_CLIENT_CLIENT_BUFFER_H_

#include "mir/graphics/native_buffer.h"
#include "mir_toolkit/common.h"
#include "mir/geometry/size.h"

#include <memory>

/**
 * \addtogroup mir_toolkit
 * @{
 */
struct MirBufferPackage;
/**@}*/

namespace mir
{

namespace client
{

/* vaddr is valid from vaddr[0] to vaddr[stride.as_uint32_t() * height.as_uint32_t() - 1] */
struct MemoryRegion
{
    geometry::Width width;
    geometry::Height height;
    geometry::Stride stride;
    MirPixelFormat format;
    std::shared_ptr<char> vaddr;
};

class ClientBuffer
{
public:
    virtual ~ClientBuffer() = default;

    virtual std::shared_ptr<MemoryRegion> secure_for_cpu_write() = 0;
    virtual geometry::Size size() const = 0;
    virtual geometry::Stride stride() const = 0;
    virtual MirPixelFormat pixel_format() const = 0;
    virtual uint32_t age() const = 0;
    virtual void increment_age() = 0;
    virtual void mark_as_submitted() = 0;
    virtual std::shared_ptr<graphics::NativeBuffer> native_buffer_handle() const = 0;

protected:
    ClientBuffer() = default;
    ClientBuffer(ClientBuffer const&) = delete;
    ClientBuffer& operator=(ClientBuffer const&) = delete;
};

}
}

#endif /* MIR_CLIENT_CLIENT_BUFFER_H_ */
