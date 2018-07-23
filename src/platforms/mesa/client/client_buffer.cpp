/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
 * Authored by: Kevin DuBois<kevin.dubois@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include "client_buffer.h"
#include "buffer_file_ops.h"
#include "gbm_format_conversions.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include <stdexcept>

#include <errno.h>
#include <sys/mman.h>

namespace mcl=mir::client;
namespace mclm=mir::client::mesa;
namespace geom=mir::geometry;

namespace
{

struct NullDeleter
{
    void operator()(char *) const {}
};

struct ShmMemoryRegion : mcl::MemoryRegion
{
    ShmMemoryRegion(std::shared_ptr<mclm::BufferFileOps> const& buffer_file_ops,
                    int buffer_fd, geom::Size const& size_param,
                    geom::Stride stride_param, MirPixelFormat format_param)
        : buffer_file_ops{buffer_file_ops},
          size_in_bytes{size_param.height.as_uint32_t() * stride_param.as_uint32_t()}
    {
        static off_t const map_offset = 0;
        width = size_param.width;
        height = size_param.height;
        stride = stride_param;
        format = format_param;

        void* map = buffer_file_ops->map(buffer_fd, map_offset, size_in_bytes);
        if (map == MAP_FAILED)
        {
            std::string msg("Failed to mmap buffer");
            BOOST_THROW_EXCEPTION(
                boost::enable_error_info(
                    std::runtime_error(msg)) << boost::errinfo_errno(errno));
        }

        vaddr = std::shared_ptr<char>(static_cast<char*>(map), NullDeleter());
    }

    ~ShmMemoryRegion()
    {
        buffer_file_ops->unmap(vaddr.get(), size_in_bytes);
    }

    std::shared_ptr<mclm::BufferFileOps> const buffer_file_ops;
    size_t const size_in_bytes;
};

std::shared_ptr<mir::graphics::mesa::NativeBuffer> to_native_buffer(
    MirBufferPackage const& package, bool gbm, uint32_t native_format, uint32_t native_flags)
{
    auto buffer = std::make_shared<mir::graphics::mesa::NativeBuffer>();
    *static_cast<MirBufferPackage*>(buffer.get()) = package;
    buffer->is_gbm_buffer = gbm;
    buffer->native_format = native_format;
    buffer->native_flags = native_flags;
    return buffer;
}

}

mclm::ClientBuffer::ClientBuffer(
    std::shared_ptr<mclm::BufferFileOps> const& buffer_file_ops,
    std::shared_ptr<MirBufferPackage> const& package,
    geom::Size size, MirPixelFormat pf) :
    buffer_file_ops{buffer_file_ops},
    creation_package{to_native_buffer(*package, false, 0, 0)},
    rect({geom::Point{0, 0}, size}),
    buffer_pf{pf},
    egl_image_attrs{
        EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
        EGL_WIDTH, static_cast<EGLint>(creation_package->width),
        EGL_HEIGHT, static_cast<EGLint>(creation_package->height),
        EGL_LINUX_DRM_FOURCC_EXT, static_cast<EGLint>(mir::graphics::mesa::mir_format_to_gbm_format(buffer_pf)),
        EGL_DMA_BUF_PLANE0_FD_EXT, creation_package->fd[0],
        EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
        EGL_DMA_BUF_PLANE0_PITCH_EXT, static_cast<EGLint>(creation_package->stride),
        EGL_NONE}
{
    if (pf == mir_pixel_format_invalid)
        BOOST_THROW_EXCEPTION(std::invalid_argument("cannot create buffer with mir_pixel_format_invalid"));
        
    if (package->fd_items != 1)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error(
            "Buffer package does not contain the expected number of fd items"));
    }
}

mclm::ClientBuffer::ClientBuffer(
    std::shared_ptr<BufferFileOps> const& buffer_file_ops,
    std::shared_ptr<MirBufferPackage> const& package,
    geometry::Size size,
    unsigned int native_pf, unsigned int native_flags) :
    buffer_file_ops{buffer_file_ops},
    creation_package{to_native_buffer(*package, true, native_pf, native_flags)},
    rect({geom::Point{0, 0}, size}),
    buffer_pf{mir::graphics::mesa::gbm_format_to_mir_format(native_pf)},
    egl_image_attrs{
        EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
        EGL_WIDTH, static_cast<EGLint>(creation_package->width),
        EGL_HEIGHT, static_cast<EGLint>(creation_package->height),
        EGL_LINUX_DRM_FOURCC_EXT, static_cast<EGLint>(native_pf),
        EGL_DMA_BUF_PLANE0_FD_EXT, creation_package->fd[0],
        EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
        EGL_DMA_BUF_PLANE0_PITCH_EXT, static_cast<EGLint>(creation_package->stride),
        EGL_NONE}
{
}

mclm::ClientBuffer::~ClientBuffer() noexcept
{
    // TODO (@raof): Error reporting? It should not be possible for this to fail; if it does,
    //               something's seriously wrong.
    buffer_file_ops->close(creation_package->fd[0]);
}

std::shared_ptr<mcl::MemoryRegion> mclm::ClientBuffer::secure_for_cpu_write()
{
    int const buffer_fd = creation_package->fd[0];

    return std::make_shared<ShmMemoryRegion>(buffer_file_ops,
                                             buffer_fd,
                                             size(),
                                             stride(),
                                             pixel_format());
}

geom::Size mclm::ClientBuffer::size() const
{
    return rect.size;
}

geom::Stride mclm::ClientBuffer::stride() const
{
    return geom::Stride{creation_package->stride};
}

MirPixelFormat mclm::ClientBuffer::pixel_format() const
{
    return buffer_pf;
}

std::shared_ptr<mir::graphics::NativeBuffer> mclm::ClientBuffer::native_buffer_handle() const
{
    creation_package->age = age();
    return creation_package;
}

void mclm::ClientBuffer::update_from(MirBufferPackage const&)
{
}

void mclm::ClientBuffer::fill_update_msg(MirBufferPackage& package)
{
    package.data_items = 0;
    package.fd_items = 0;
}

MirBufferPackage* mclm::ClientBuffer::package() const
{
    if (auto native = dynamic_cast<mir::graphics::mesa::NativeBuffer*>(native_buffer_handle().get()))
        return native;
    BOOST_THROW_EXCEPTION(std::invalid_argument("could not convert NativeBuffer"));
}

void mclm::ClientBuffer::egl_image_creation_parameters(
    EGLenum* type, EGLClientBuffer* client_buffer, EGLint** attrs)
{
    *attrs = egl_image_attrs.data();
    *type = EGL_LINUX_DMA_BUF_EXT;
    *client_buffer = nullptr;
}
