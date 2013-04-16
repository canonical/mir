/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_ANDROID_BUFFER_HANDLE_H_
#define MIR_GRAPHICS_ANDROID_ANDROID_BUFFER_HANDLE_H_

#include "mir/geometry/size.h"
#include "mir/geometry/pixel_format.h"

#include <EGL/egl.h>
#include <memory>

namespace mir
{
namespace compositor
{
class BufferIPCPackage;
}

namespace graphics
{
namespace android
{

enum class BufferUsage : uint32_t
{
    use_hardware, //buffer supports usage as a gles render target, and a gles texture
    use_software, //buffer supports usage as a cpu render target, and a gles texture
    use_framebuffer_gles //buffer supports usage as a gles render target, hwc layer, and is postable to framebuffer
};

/* note: this interface will need a new concrete class implementing it when the struct for ANativeWindowBuffer changes */
class AndroidBufferHandle
{
public:
    virtual ~AndroidBufferHandle() {}

    virtual geometry::Size size() const   = 0;
    virtual geometry::Stride stride() const = 0;
    virtual geometry::PixelFormat format() const  = 0;
    virtual BufferUsage usage() const = 0;

    virtual EGLClientBuffer get_egl_client_buffer() const = 0;
    virtual std::shared_ptr<compositor::BufferIPCPackage> get_ipc_package() const = 0;

protected:
    AndroidBufferHandle() = default;
};

}
}
}

#endif /*MIR_GRAPHICS_ANDROID_ANDROID_BUFFER_HANDLE_H_ */
