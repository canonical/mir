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

#ifndef MIR_CLIENT_GBM_CLIENT_BUFFER_H_
#define MIR_CLIENT_GBM_CLIENT_BUFFER_H_

#include "../aging_buffer.h"
#include "mir_toolkit/mir_client_library.h"
#include "mir/geometry/rectangle.h"

#include <memory>

namespace mir
{
namespace client
{
namespace gbm
{

class BufferFileOps;

class ClientBuffer : public AgingBuffer
{
public:
    ClientBuffer(std::shared_ptr<BufferFileOps> const& buffer_file_ops,
                 std::shared_ptr<MirBufferPackage> const& buffer_package,
                 geometry::Size size,
                 MirPixelFormat pf);

    ~ClientBuffer() noexcept;

    std::shared_ptr<MemoryRegion> secure_for_cpu_write();
    geometry::Size size() const;
    geometry::Stride stride() const;
    MirPixelFormat pixel_format() const;
    std::shared_ptr<MirNativeBuffer> native_buffer_handle() const;

private:
    std::shared_ptr<BufferFileOps> const buffer_file_ops;
    std::shared_ptr<MirBufferPackage> const creation_package;
    geometry::Rectangle const rect;
    MirPixelFormat const buffer_pf;
};

}
}
}
#endif /* MIR_CLIENT_GBM_CLIENT_BUFFER_H_ */
