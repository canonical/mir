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
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_ANDROID_ANDROID_BUFFER_HANDLE_H_
#define MIR_GRAPHICS_ANDROID_ANDROID_BUFFER_HANDLE_H_

#include "mir/geometry/dimensions.h"
#include "mir/compositor/pixel_format.h"

#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <system/window.h>

namespace mir
{
namespace graphics
{
namespace android
{

enum class BufferUsage : uint32_t
{
    use_hardware,
    use_software
};

class BufferHandle 
{
public:
    virtual ~BufferHandle() {}

    virtual geometry::Width width() = 0;
    virtual geometry::Height height() = 0;
    virtual geometry::Stride stride() = 0;
    virtual compositor::PixelFormat format() = 0;
    virtual BufferUsage usage() = 0;

    virtual EGLClientBuffer get_egl_client_buffer() = 0;

protected:
    BufferHandle() = default;
};


class AndroidBufferHandle: public BufferHandle
{
public:
    explicit AndroidBufferHandle(buffer_handle_t han, geometry::Width w, geometry::Height h,
                                              geometry::Stride s, int pf, int use);

    geometry::Width width();
    geometry::Height height();
    geometry::Stride stride();
    compositor::PixelFormat format();
    BufferUsage usage();

    EGLClientBuffer get_egl_client_buffer();


    ANativeWindowBuffer anw_buffer;
    buffer_handle_t handle;
};

}
}
}

#endif /*MIR_GRAPHICS_ANDROID_ANDROID_BUFFER_HANDLE_H_ */
