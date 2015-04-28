/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 *
 */

#include "buffer_allocator.h"
#include "gbm_buffer.h"
#include "buffer_texture_binder.h"
#include "anonymous_shm_file.h"
#include "shm_buffer.h"
#include "mir/graphics/egl_extensions.h"
#include "mir/graphics/buffer_properties.h"
#include <boost/throw_exception.hpp>
#include <boost/exception/errinfo_errno.hpp>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <algorithm>
#include <fcntl.h>
#include <xf86drm.h>

#include "../debug.h"

namespace mg  = mir::graphics;
namespace mgx = mg::X;
namespace geom = mir::geometry;

namespace
{

class EGLImageBufferTextureBinder : public mgx::BufferTextureBinder
{
public:
    EGLImageBufferTextureBinder(std::shared_ptr<gbm_bo> const& gbm_bo,
                                std::shared_ptr<mg::EGLExtensions> const& egl_extensions)
        : bo{gbm_bo}, egl_extensions{egl_extensions}, egl_image{EGL_NO_IMAGE_KHR}, prime_fd{-1}
    {
    }

    ~EGLImageBufferTextureBinder()
    {
        if (egl_image != EGL_NO_IMAGE_KHR)
            egl_extensions->eglDestroyImageKHR(egl_display, egl_image);
        if (prime_fd > 0)
            close(prime_fd);
    }


    void gl_bind_to_texture() override
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

            // TODO: prime_fd could come from gbm_buffer
            auto device = gbm_bo_get_device(bo_raw);
            auto gem_handle = gbm_bo_get_handle(bo_raw).u32;
            auto drm_fd = gbm_device_get_fd(device);

            auto ret = drmPrimeHandleToFD(drm_fd, gem_handle, DRM_CLOEXEC, &prime_fd);
            if (ret)
            {
                std::string const msg("Failed to get PRIME fd from gbm bo");
                BOOST_THROW_EXCEPTION(
                    boost::enable_error_info(
                        std::runtime_error(msg)) << boost::errinfo_errno(errno));
            }

            const EGLint image_attrs[] =
            {
                EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,

                EGL_WIDTH, (const EGLint)gbm_bo_get_width(bo_raw),
                EGL_HEIGHT, (const EGLint)gbm_bo_get_height(bo_raw),
                EGL_LINUX_DRM_FOURCC_EXT, (const EGLint)gbm_bo_get_format(bo_raw),
                EGL_DMA_BUF_PLANE0_FD_EXT, prime_fd,
                EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
                EGL_DMA_BUF_PLANE0_PITCH_EXT, (const EGLint)gbm_bo_get_stride(bo_raw),
                EGL_NONE
            };

            egl_image = egl_extensions->eglCreateImageKHR(egl_display,
                                                          EGL_NO_CONTEXT,
                                                          EGL_LINUX_DMA_BUF_EXT,
                                                          (EGLClientBuffer)NULL,
                                                          image_attrs);
            if (egl_image == EGL_NO_IMAGE_KHR)
                BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create EGLImage from GBM bo"));
        }
    }

    std::shared_ptr<gbm_bo> const bo;
    std::shared_ptr<mg::EGLExtensions> const egl_extensions;
    EGLDisplay egl_display;
    EGLImageKHR egl_image;
    int prime_fd;
};

struct GBMBODeleter
{
    void operator()(gbm_bo* handle) const
    {
        if (handle)
            gbm_bo_destroy(handle);
    }
};

}

mgx::BufferAllocator::BufferAllocator(
    gbm_device* device)
    : device(device),
      egl_extensions(std::make_shared<mg::EGLExtensions>())
{
}

std::shared_ptr<mg::Buffer> mgx::BufferAllocator::alloc_buffer(
    BufferProperties const& buffer_properties)
{
	std::shared_ptr<mg::Buffer> buffer;

    CALLED
    if (buffer_properties.usage == BufferUsage::software)
    {
        mir::log_info("\t\t software buffer");
        buffer = alloc_software_buffer(buffer_properties);
    }
    else
    {
        mir::log_info("\t\t hardware buffer");
        buffer = alloc_hardware_buffer(buffer_properties);
    }

    return buffer;
}

std::shared_ptr<mg::Buffer> mgx::BufferAllocator::alloc_hardware_buffer(
    BufferProperties const& buffer_properties)
{
//    return alloc_software_buffer(buffer_properties);
    uint32_t bo_flags{GBM_BO_USE_RENDERING};

    uint32_t const gbm_format = mgx::mir_format_to_gbm_format(buffer_properties.format);

    if (!is_pixel_format_supported(buffer_properties.format) ||
        gbm_format == mgx::invalid_gbm_format)
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error("Trying to create GBM buffer with unsupported pixel format"));
    }

    gbm_bo *bo_raw = gbm_bo_create(
        device,
        buffer_properties.size.width.as_uint32_t(),
        buffer_properties.size.height.as_uint32_t(),
        gbm_format,
        bo_flags);

    if (!bo_raw)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create GBM buffer object"));

    std::shared_ptr<gbm_bo> bo{bo_raw, GBMBODeleter()};

    std::unique_ptr<EGLImageBufferTextureBinder> texture_binder{
        new EGLImageBufferTextureBinder{bo, egl_extensions}};

    /* Create the GBMBuffer */
    auto const buffer =
        std::make_shared<GBMBuffer>(bo, bo_flags, std::move(texture_binder));

    return buffer;
}

std::shared_ptr<mg::Buffer> mgx::BufferAllocator::alloc_software_buffer(
    BufferProperties const& buffer_properties)
{
    if (!is_pixel_format_supported(buffer_properties.format))
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error(
                "Trying to create SHM buffer with unsupported pixel format"));
    }

    auto const stride = geom::Stride{
        MIR_BYTES_PER_PIXEL(buffer_properties.format) *
        buffer_properties.size.width.as_uint32_t()};
    size_t const size_in_bytes =
        stride.as_int() * buffer_properties.size.height.as_int();
    auto const shm_file =
        std::make_shared<mgx::AnonymousShmFile>(size_in_bytes);

    auto const buffer =
        std::make_shared<ShmBuffer>(shm_file, buffer_properties.size,
                                    buffer_properties.format);

    return buffer;
}

std::vector<MirPixelFormat> mgx::BufferAllocator::supported_pixel_formats()
{
    CALLED
    static std::vector<MirPixelFormat> const pixel_formats{
        mir_pixel_format_argb_8888,
        mir_pixel_format_xrgb_8888,
        mir_pixel_format_bgr_888
    };

    return pixel_formats;
}

bool mgx::BufferAllocator::is_pixel_format_supported(MirPixelFormat format)
{
    auto formats = supported_pixel_formats();

    auto iter = std::find(formats.begin(), formats.end(), format);

    return iter != formats.end();
}
