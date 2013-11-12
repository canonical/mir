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

#include "mir/graphics/egl_extensions.h"
#include "mir/graphics/android/native_buffer.h"
#include "mir/graphics/android/sync_fence.h"
#include "android_format_conversion-inl.h"
#include "buffer.h"

#include <system/window.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

mga::Buffer::Buffer(std::shared_ptr<NativeBuffer> const& buffer_handle,
                    std::shared_ptr<mg::EGLExtensions> const& extensions)
    : native_buffer(buffer_handle),
      egl_extensions(extensions)
{
}

mga::Buffer::~Buffer()
{
    std::map<EGLDisplay,EGLImageKHR>::iterator it;
    for(it = egl_image_map.begin(); it != egl_image_map.end(); it++)
    {
        egl_extensions->eglDestroyImageKHR(it->first, it->second);
    }
}


geom::Size mga::Buffer::size() const
{
    ANativeWindowBuffer *anwb = native_buffer->anwb();
    return {anwb->width, anwb->height};
}

geom::Stride mga::Buffer::stride() const
{
    ANativeWindowBuffer *anwb = native_buffer->anwb();
    return geom::Stride{anwb->stride *
                        geom::bytes_per_pixel(pixel_format())};
}

geom::PixelFormat mga::Buffer::pixel_format() const
{
    ANativeWindowBuffer *anwb = native_buffer->anwb();
    return mga::to_mir_format(anwb->format);
}

bool mga::Buffer::can_bypass() const
{
    return false;
}

void mga::Buffer::bind_to_texture()
{
    std::unique_lock<std::mutex> lk(content_lock);
    native_buffer->wait_for_content();

    EGLDisplay disp = eglGetCurrentDisplay();
    if (disp == EGL_NO_DISPLAY) {
        BOOST_THROW_EXCEPTION(std::runtime_error("cannot bind buffer to texture without EGL context\n"));
    }
    static const EGLint image_attrs[] =
    {
        EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
        EGL_NONE
    };
    EGLImageKHR image;
    auto it = egl_image_map.find(disp);
    if (it == egl_image_map.end())
    {
        image = egl_extensions->eglCreateImageKHR(disp, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
                                  native_buffer->anwb(), image_attrs);
        if (image == EGL_NO_IMAGE_KHR)
        {
            BOOST_THROW_EXCEPTION(std::runtime_error("error binding buffer to texture\n"));
        }
        egl_image_map[disp] = image;
    }
    else /* already had it in map */
    {
        image = it->second;
    }

    egl_extensions->glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);

    //TODO: we should make use of the android egl fence extension here to update the fence.
    //      if the extension is not available, we should pass out a token that the user
    //      will have to keep until the completion of the gl draw
}

std::shared_ptr<mg::NativeBuffer> mga::Buffer::native_buffer_handle() const
{
    std::unique_lock<std::mutex> lk(content_lock);

    auto native_resource = std::shared_ptr<mg::NativeBuffer>(
        native_buffer.get(),
        [this](NativeBuffer*)
        {
            content_lock.unlock();
        });

    //lock remains in effect until the native handle is released 
    lk.release();
    return native_resource;
}
