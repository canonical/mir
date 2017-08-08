/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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

#ifndef MIR_CLIENT_MESA_CLIENT_BUFFER_H_
#define MIR_CLIENT_MESA_CLIENT_BUFFER_H_

#include "mir/aging_buffer.h"
#include "mir_toolkit/mir_client_library.h"
#include "mir/geometry/rectangle.h"
#include "native_buffer.h"

#include <vector>
#include <memory>

namespace mir
{
namespace client
{
namespace mesa
{

class BufferFileOps;

class ClientBuffer : public AgingBuffer
{
public:
    ClientBuffer(std::shared_ptr<BufferFileOps> const& buffer_file_ops,
                 std::shared_ptr<MirBufferPackage> const& buffer_package,
                 geometry::Size size,
                 MirPixelFormat pf);
    ClientBuffer(std::shared_ptr<BufferFileOps> const& buffer_file_ops,
                 std::shared_ptr<MirBufferPackage> const& buffer_package,
                 geometry::Size size,
                 unsigned int native_pf, unsigned int native_flags);

    ~ClientBuffer() noexcept;

    std::shared_ptr<MemoryRegion> secure_for_cpu_write();
    geometry::Size size() const;
    geometry::Stride stride() const;
    MirPixelFormat pixel_format() const;
    std::shared_ptr<graphics::NativeBuffer> native_buffer_handle() const;
    void update_from(MirBufferPackage const&);
    void fill_update_msg(MirBufferPackage&);
    MirBufferPackage* package() const;
    void egl_image_creation_parameters(EGLenum*, EGLClientBuffer*, EGLint**);

private:
    std::shared_ptr<BufferFileOps> const buffer_file_ops;
    std::shared_ptr<graphics::mesa::NativeBuffer> const creation_package;
    geometry::Rectangle const rect;
    MirPixelFormat const buffer_pf;
    std::vector<EGLint> egl_image_attrs;
};

}
}
}
#endif /* MIR_CLIENT_MESA_CLIENT_BUFFER_H_ */
