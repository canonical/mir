/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_NESTED_HOST_BUFFER_H_
#define MIR_GRAPHICS_NESTED_HOST_BUFFER_H_

#include "mir/graphics/buffer_properties.h"
#include "mir_toolkit/mir_buffer.h"
#include "mir_toolkit/extensions/gbm_buffer.h"
#include "mir_toolkit/extensions/android_buffer.h"
#include "native_buffer.h"
#include <functional>
#include <mutex>
#include <condition_variable>

namespace mir
{
namespace graphics
{
namespace nested
{
class HostBuffer : public NativeBuffer
{
public:
    HostBuffer(MirConnection* mir_connection, BufferProperties const& properties);
    HostBuffer(MirConnection* mir_connection, geometry::Size size, MirPixelFormat format);
    HostBuffer(MirConnection* mir_connection,
        MirExtensionGbmBufferV1 const* ext,
        geometry::Size size, unsigned int native_pf, unsigned int native_flags);
    HostBuffer(MirConnection* mir_connection,
        MirExtensionAndroidBufferV1 const* ext,
        geometry::Size size, unsigned int native_pf, unsigned int native_flags);
    ~HostBuffer();

    void sync(MirBufferAccess access, std::chrono::nanoseconds ns) override;
    MirBuffer* client_handle() const override;
    std::unique_ptr<GraphicsRegion> get_graphics_region() override;
    geometry::Size size() const override;
    MirPixelFormat format() const override;
    std::tuple<EGLenum, EGLClientBuffer, EGLint*> egl_image_creation_hints() const override;
    static void buffer_available(MirBuffer* buffer, void* context);
    void available(MirBuffer* buffer) override;
    void on_ownership_notification(std::function<void()> const& fn) override;
    MirBufferPackage* package() const override;
    void set_fence(mir::Fd fd) override;
    mir::Fd fence() const override;

private:
    MirExtensionFencedBuffersV1 const* fence_extensions = nullptr;
    std::function<void()> f;
    MirBuffer* handle = nullptr;
    std::mutex mut;
    std::condition_variable cv;
};
}
}
}
#endif /* MIR_GRAPHICS_NESTED_HOST_BUFFER_H_ */
