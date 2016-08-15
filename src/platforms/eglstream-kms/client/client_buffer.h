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
 * Authored by:
 *   Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_CLIENT_EGLSTREAM_CLIENT_BUFFER_H_
#define MIR_CLIENT_EGLSTREAM_CLIENT_BUFFER_H_

#include "mir/aging_buffer.h"
#include "mir_toolkit/mir_client_library.h"
#include "mir/geometry/rectangle.h"

#include <memory>

namespace mir
{
namespace client
{
namespace eglstream
{

class ClientBuffer : public AgingBuffer
{
public:
    ClientBuffer(
        std::shared_ptr<MirBufferPackage> const& buffer_package,
        geometry::Size size,
        MirPixelFormat pf);

    ~ClientBuffer() noexcept;

    std::shared_ptr<MemoryRegion> secure_for_cpu_write();
    geometry::Size size() const;
    geometry::Stride stride() const;
    MirPixelFormat pixel_format() const;
    std::shared_ptr<MirNativeBuffer> native_buffer_handle() const;
    void update_from(MirBufferPackage const&);
    void fill_update_msg(MirBufferPackage&);
    MirNativeBuffer* as_mir_native_buffer() const;
    void set_fence(MirNativeFence*, MirBufferAccess);
    MirNativeFence* get_fence() const;
    bool wait_fence(MirBufferAccess, std::chrono::nanoseconds timeout);

private:
    std::shared_ptr<MirBufferPackage> const creation_package;
    geometry::Rectangle const rect;
    MirPixelFormat const buffer_pf;
};

}
}
}
#endif /* MIR_CLIENT_EGLSTREAM_CLIENT_BUFFER_H_ */
