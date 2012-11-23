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
 *   Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "gbm_buffer_allocator.h"
#include "gbm_buffer.h"
#include "gbm_platform.h"
#include "buffer_texture_binder.h"
#include "mir/graphics/buffer_initializer.h"
#include "mir/compositor/buffer_properties.h"
#include "mir/exception.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdexcept>
#include <xf86drm.h>
#include <gbm.h>
#include <cassert>

namespace mg  = mir::graphics;
namespace mgg = mir::graphics::gbm;
namespace mc  = mir::compositor;
namespace geom = mir::geometry;

namespace mir
{
namespace graphics
{
namespace gbm
{

struct EGLExtensions
{
    EGLExtensions()
        : eglCreateImageKHR{
              reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(
                  eglGetProcAddress("eglCreateImageKHR"))},
          eglDestroyImageKHR{
              reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(
                  eglGetProcAddress("eglDestroyImageKHR"))},
          glEGLImageTargetTexture2DOES{
              reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(
                  eglGetProcAddress("glEGLImageTargetTexture2DOES"))}
    {
        if (!eglCreateImageKHR || !eglDestroyImageKHR)
            throw std::runtime_error("EGL implementation doesn't support EGLImage");

        if (!glEGLImageTargetTexture2DOES)
            throw std::runtime_error("GLES2 implementation doesn't support updating a texture from an EGLImage");
    }

    PFNEGLCREATEIMAGEKHRPROC const eglCreateImageKHR;
    PFNEGLDESTROYIMAGEKHRPROC const eglDestroyImageKHR;
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC const glEGLImageTargetTexture2DOES;
};

}
}
}

namespace
{

class EGLImageBufferTextureBinder : public mgg::BufferTextureBinder
{
public:
    EGLImageBufferTextureBinder(std::shared_ptr<gbm_bo> const& gbm_bo,
                                uint32_t gem_flink_name,
                                std::shared_ptr<mgg::EGLExtensions> const& egl_extensions)
        : bo{gbm_bo}, gem_flink_name{gem_flink_name},
          egl_extensions{egl_extensions}, egl_image{EGL_NO_IMAGE_KHR}
    {
    }

    ~EGLImageBufferTextureBinder()
    {
        if (egl_image != EGL_NO_IMAGE_KHR)
            egl_extensions->eglDestroyImageKHR(egl_display, egl_image);
    }


    void bind_to_texture()
    {
        ensure_egl_image();

        egl_extensions->glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, egl_image);
    }

private:
    void ensure_egl_image()
    {
        if (egl_image == EGL_NO_IMAGE_KHR)
        {
            egl_display = eglGetCurrentDisplay();
            gbm_bo* bo_raw{bo.get()};

            auto stride = gbm_bo_get_stride(bo_raw);
            auto width = gbm_bo_get_width(bo_raw);
            auto height = gbm_bo_get_height(bo_raw);

            const EGLint image_attrs[] =
            {
                EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
                EGL_WIDTH, static_cast<EGLint>(width),
                EGL_HEIGHT, static_cast<EGLint>(height),
                EGL_DRM_BUFFER_FORMAT_MESA, EGL_DRM_BUFFER_FORMAT_ARGB32_MESA,
                EGL_DRM_BUFFER_STRIDE_MESA, static_cast<EGLint>(stride) / 4,
                EGL_NONE
            };

            egl_image = egl_extensions->eglCreateImageKHR(egl_display, EGL_NO_CONTEXT,
                                                          EGL_DRM_BUFFER_MESA,
                                                          reinterpret_cast<void*>(gem_flink_name),
                                                          image_attrs);
            if (egl_image == EGL_NO_IMAGE_KHR)
                throw std::runtime_error("Failed to create EGLImage from GBM bo");
        }
    }

    std::shared_ptr<gbm_bo> const bo;
    uint32_t const gem_flink_name;
    std::shared_ptr<mgg::EGLExtensions> const egl_extensions;
    EGLDisplay egl_display;
    EGLImageKHR egl_image;
};

}

mgg::GBMBufferAllocator::GBMBufferAllocator(
        const std::shared_ptr<GBMPlatform>& platform,
        const std::shared_ptr<BufferInitializer>& buffer_initializer)
        : platform(platform), buffer_initializer(buffer_initializer),
          egl_extensions(std::make_shared<EGLExtensions>())
{
    assert(buffer_initializer.get() != 0);
}

std::unique_ptr<mc::Buffer> mgg::GBMBufferAllocator::alloc_buffer(
    mc::BufferProperties const& buffer_properties)
{
    uint32_t bo_flags{0};

    /* Create the GBM buffer object */
    if (buffer_properties.usage == mc::BufferUsage::software)
        bo_flags |= GBM_BO_USE_WRITE;
    else
        bo_flags |= GBM_BO_USE_RENDERING;

    gbm_bo *bo_raw = gbm_bo_create(
        platform->gbm.device, 
        buffer_properties.size.width.as_uint32_t(),
        buffer_properties.size.height.as_uint32_t(),
        mgg::mir_format_to_gbm_format(buffer_properties.format),
        bo_flags);
    
    if (!bo_raw)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create GBM buffer object"));

    std::shared_ptr<gbm_bo> bo{bo_raw, mgg::GBMBufferObjectDeleter()};

    /* Get the GEM flink name from the GBM buffer object */
    auto gem_handle = gbm_bo_get_handle(bo_raw).u32;
    auto drm_fd = gbm_device_get_fd(platform->gbm.device);
    struct drm_gem_flink flink;
    flink.handle = gem_handle;

    auto ret = drmIoctl(drm_fd, DRM_IOCTL_GEM_FLINK, &flink);
    if (ret)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to get GEM flink name from gbm bo"));

    std::unique_ptr<EGLImageBufferTextureBinder> texture_binder{
        new EGLImageBufferTextureBinder{bo, flink.name, egl_extensions}};

    /* Create the GBMBuffer */
    std::unique_ptr<mc::Buffer> buffer{new GBMBuffer{bo, std::move(texture_binder), flink.name}};

    (*buffer_initializer)(*buffer);

    return buffer;
}
