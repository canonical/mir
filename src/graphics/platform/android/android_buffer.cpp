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

#include "mir/graphics/android/android_buffer.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

namespace mc=mir::compositor;
namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

mga::AndroidBuffer::AndroidBuffer(const std::shared_ptr<GraphicAllocAdaptor>& alloc_dev,
                                 geom::Width w, geom::Height h, mc::PixelFormat pf)
    :
    alloc_device(alloc_dev)
{
    BufferUsage usage = mga::BufferUsage::use_hardware;

    if (!alloc_device)
        throw std::runtime_error("No allocation device for graphics buffer");

    native_window_buffer_handle = alloc_device->alloc_buffer(w, h,
                                      pf, usage);
    
    if (!native_window_buffer_handle.get())
        throw std::runtime_error("Graphics buffer allocation failed");

}

mga::AndroidBuffer::~AndroidBuffer()
{
    std::map<EGLDisplay,EGLImageKHR>::iterator it;
    for(it = egl_image_map.begin(); it != egl_image_map.end(); it++)
    {
        eglDestroyImageKHR(it->first, it->second); 
    }
}


geom::Width mga::AndroidBuffer::width() const
{
    return native_window_buffer_handle->width();
}

geom::Height mga::AndroidBuffer::height() const
{
    return native_window_buffer_handle->height();
}

geom::Stride mga::AndroidBuffer::stride() const
{
    return geom::Stride(0);
}

mc::PixelFormat mga::AndroidBuffer::pixel_format() const
{
    return native_window_buffer_handle->format();
}

void mga::AndroidBuffer::lock()
{
}

void mga::AndroidBuffer::unlock()
{
}

mg::Texture* mga::AndroidBuffer::bind_to_texture()
{
    std::map<EGLDisplay,EGLImageKHR>::iterator it;
    EGLDisplay disp = eglGetCurrentDisplay();

    static const EGLint image_attrs[] = {
        EGL_IMAGE_PRESERVED_KHR,    EGL_TRUE,
        EGL_NONE
    };

    EGLImageKHR image;
    it = egl_image_map.find(disp);
    if (it == egl_image_map.end()) {
        image = eglCreateImageKHR(disp, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
                                  native_window_buffer_handle->get_egl_client_buffer() , image_attrs);

        if (image == EGL_NO_IMAGE_KHR)
        {
            return NULL;
        }
        egl_image_map[disp] = image;
    } 
    else /* already had it in map */
    {
        image = it->second; 
    }

    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image); 

    return NULL;
}
