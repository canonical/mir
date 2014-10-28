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

#ifndef MIR_CLIENT_ANDROID_ANDROID_CLIENT_BUFFER_H_
#define MIR_CLIENT_ANDROID_ANDROID_CLIENT_BUFFER_H_

#include "mir/graphics/android/android_native_buffer.h"
#include "../aging_buffer.h"

#include <system/window.h>
#include <memory>

namespace mir
{
namespace client
{
namespace android
{

class BufferRegistrar;
class Buffer : public AgingBuffer
{
public:
    Buffer(
        std::shared_ptr<BufferRegistrar> const& registrar,
        MirBufferPackage const& package,
        MirPixelFormat pf);

    std::shared_ptr<MemoryRegion> secure_for_cpu_write() override;
    geometry::Size size() const override;
    geometry::Stride stride() const override;
    MirPixelFormat pixel_format() const override;
    std::shared_ptr<graphics::NativeBuffer> native_buffer_handle() const override;
    void update_from(MirBufferPackage const& update_package) override;
    void fill_update_msg(MirBufferPackage& message) override;

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
private:
    void pack_native_window_buffer();

    std::shared_ptr<BufferRegistrar> const buffer_registrar;
    std::shared_ptr<graphics::NativeBuffer> const native_buffer;
    MirPixelFormat const buffer_pf;
    geometry::Stride const buffer_stride;
    geometry::Size const buffer_size;
};

}
}
}
#endif /* MIR_CLIENT_ANDROID_ANDROID_CLIENT_BUFFER_H_ */
