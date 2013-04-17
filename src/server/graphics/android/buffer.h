/*
 * Copyright © 2012,2013 Canonical Ltd.
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

#ifndef MIR_GRAPHICS_ANDROID_BUFFER_H_
#define MIR_GRAPHICS_ANDROID_BUFFER_H_

#include "android_buffer.h"
#include "android_buffer_handle.h"

#include <map>

#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace mir
{
namespace graphics
{
namespace android
{

class GraphicAllocAdaptor;
class Buffer: public AndroidBuffer 
{
public:
    Buffer(const std::shared_ptr<GraphicAllocAdaptor>& device,
                  geometry::Size size, geometry::PixelFormat pf, BufferUsage use);
    ~Buffer();

    /* from BufferBasic */
    geometry::Size size() const;
    geometry::Stride stride() const;
    geometry::PixelFormat pixel_format() const;
    std::shared_ptr<compositor::BufferIPCPackage> get_ipc_package() const;
    void bind_to_texture();

    /* android-specific */
    std::shared_ptr<ANativeWindowBuffer> native_buffer_handle() const;
private:
    std::shared_ptr<GraphicAllocAdaptor> const alloc_device;

    std::map<EGLDisplay,EGLImageKHR> egl_image_map;

    std::shared_ptr<AndroidBufferHandle> native_window_buffer_handle;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_BUFFER_H_ */
