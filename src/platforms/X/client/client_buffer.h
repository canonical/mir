/*
 * Copyright © 2015 Canonical Ltd.
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
 *   Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#ifndef MIR_CLIENT_X_CLIENT_BUFFER_H_
#define MIR_CLIENT_X_CLIENT_BUFFER_H_

#include "mir/aging_buffer.h"
#include "mir_toolkit/mir_client_library.h"
#include "mir/geometry/rectangle.h"

#include <memory>

namespace mir
{
namespace client
{
namespace X
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

    std::shared_ptr<MemoryRegion> secure_for_cpu_write() override;
    geometry::Size size() const override;
    geometry::Stride stride() const override;
    MirPixelFormat pixel_format() const override;
    std::shared_ptr<MirNativeBuffer> native_buffer_handle() const override;
    void update_from(MirBufferPackage const&) override;
    void fill_update_msg(MirBufferPackage&) override;

private:
    std::shared_ptr<BufferFileOps> const buffer_file_ops;
    std::shared_ptr<MirBufferPackage> const creation_package;
    geometry::Rectangle const rect;
    MirPixelFormat const buffer_pf;
};

}
}
}
#endif /* MIR_CLIENT_X_CLIENT_BUFFER_H_ */
