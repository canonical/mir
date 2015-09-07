/*
 * Copyright © 2012,2013 Canonical Ltd.
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

#ifndef MIR_GRAPHICS_ANDROID_BUFFER_H_
#define MIR_GRAPHICS_ANDROID_BUFFER_H_

#include "mir/graphics/buffer_basic.h"
#include "buffer_usage.h"

#include <hardware/gralloc.h>

#include <mutex>
#include <condition_variable>
#include <map>

#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace mir
{
namespace graphics
{
struct EGLExtensions;
namespace android
{

class Buffer: public BufferBasic, public NativeBufferBase
{
public:
    Buffer(gralloc_module_t const* hw_module,
           std::shared_ptr<NativeBuffer> const& buffer_handle,
           std::shared_ptr<EGLExtensions> const& extensions);
    ~Buffer();

    geometry::Size size() const override;
    geometry::Stride stride() const override;
    MirPixelFormat pixel_format() const override;
    void gl_bind_to_texture() override;
    //note, you will get the native representation of an android buffer, including
    //the fences associated with the buffer. You must close these fences
    std::shared_ptr<NativeBuffer> native_buffer_handle() const override;

    void write(unsigned char const* pixels, size_t size) override;
    void read(std::function<void(unsigned char const*)> const&) override;

    NativeBufferBase* native_buffer_base() override;

private:
    gralloc_module_t const* hw_module;

    typedef std::pair<EGLDisplay, EGLContext> DispContextPair;
    std::map<DispContextPair,EGLImageKHR> egl_image_map;

    std::mutex mutable content_lock;
    std::shared_ptr<NativeBuffer> native_buffer;
    std::shared_ptr<EGLExtensions> egl_extensions;
};

}
}
}

#endif /* MIR_GRAPHICS_ANDROID_BUFFER_H_ */
