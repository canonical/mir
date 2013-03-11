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
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "android_buffer.h"

#include <system/window.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <boost/throw_exception.hpp>

namespace mc=mir::compositor;
namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

mga::AndroidBuffer::AndroidBuffer(const std::shared_ptr<GraphicAllocAdaptor>& alloc_dev,
                                  geom::Size size, geom::PixelFormat pf)
    :
    alloc_device(alloc_dev)
{
    BufferUsage usage = mga::BufferUsage::use_hardware;

    if (!alloc_device)
        BOOST_THROW_EXCEPTION(std::runtime_error("No allocation device for graphics buffer"));

    native_window_buffer_handle = alloc_device->alloc_buffer(size, pf, usage);

    if (!native_window_buffer_handle.get())
        BOOST_THROW_EXCEPTION(std::runtime_error("Graphics buffer allocation failed"));

}

mga::AndroidBuffer::~AndroidBuffer()
{
    std::map<EGLDisplay,EGLImageKHR>::iterator it;
    for(it = egl_image_map.begin(); it != egl_image_map.end(); it++)
    {
        eglDestroyImageKHR(it->first, it->second);
    }
}


geom::Size mga::AndroidBuffer::size() const
{
    return native_window_buffer_handle->size();
}

geom::Stride mga::AndroidBuffer::stride() const
{
    return native_window_buffer_handle->stride();
}

geom::PixelFormat mga::AndroidBuffer::pixel_format() const
{
    return native_window_buffer_handle->format();
}

void mga::AndroidBuffer::bind_to_texture()
{
    EGLDisplay disp = eglGetCurrentDisplay();
    if (disp == EGL_NO_DISPLAY) {
        BOOST_THROW_EXCEPTION(std::runtime_error("cannot bind buffer to texture without EGL context\n"));
    }
    static const EGLint image_attrs[] =
    {
        EGL_IMAGE_PRESERVED_KHR,    EGL_TRUE,
        EGL_NONE
    };
    EGLImageKHR image;
    auto it = egl_image_map.find(disp);
    if (it == egl_image_map.end())
    {
        ANativeWindowBuffer *buf = (ANativeWindowBuffer*) native_window_buffer_handle->get_egl_client_buffer();
        image = eglCreateImageKHR(disp, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
                                  buf, image_attrs);
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

    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);

    return;
}

std::shared_ptr<mc::BufferIPCPackage> mga::AndroidBuffer::get_ipc_package() const
{
    return native_window_buffer_handle->get_ipc_package();
}
