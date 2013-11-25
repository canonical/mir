/*
 * Copyright © 2013 Canonical Ltd.
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
 * Eleni Maria Stea <elenimaria.stea@canonical.com>
 * Alan Griffiths <alan@octopull.co.uk>
 */

#include "native_gbm_platform.h"

#include "gbm_buffer_allocator.h"
#include "mir/graphics/buffer_ipc_packer.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/graphics/nested_context.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mgg = mg::gbm;

void mgg::NativeGBMPlatform::initialize(
    std::shared_ptr<NestedContext> const& nested_context_arg)
{
    nested_context = nested_context_arg;
    auto fds = nested_context->platform_fd_items();
    drm_fd = fds.at(0);
    gbm.setup(drm_fd);
    nested_context->drm_set_gbm_device(gbm.device);
}

std::shared_ptr<mg::GraphicBufferAllocator> mgg::NativeGBMPlatform::create_buffer_allocator(
        std::shared_ptr<mg::BufferInitializer> const& buffer_initializer)
{
    return std::make_shared<mgg::GBMBufferAllocator>(gbm.device, buffer_initializer);
}

std::shared_ptr<mg::PlatformIPCPackage> mgg::NativeGBMPlatform::get_ipc_package()
{
    struct NativeGBMPlatformIPCPackage : public mg::PlatformIPCPackage
    {
        NativeGBMPlatformIPCPackage(int fd)
        {
            ipc_fds.push_back(fd);
        }
    };
    char* busid = drmGetBusid(drm_fd);
    if (!busid)
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to get BusID of DRM device")) << boost::errinfo_errno(errno));
    int auth_fd = drmOpen(NULL, busid);
    free(busid);

    drm_magic_t magic;
    int ret = -1;
    if ((ret = drmGetMagic(auth_fd, &magic)) < 0)
    {
        close(auth_fd);
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Failed to get DRM device magic cookie")) << boost::errinfo_errno(-ret));
    }

    nested_context->drm_auth_magic(magic);

    return std::make_shared<NativeGBMPlatformIPCPackage>(auth_fd);
}

std::shared_ptr<mg::InternalClient> mgg::NativeGBMPlatform::create_internal_client()
{
    BOOST_THROW_EXCEPTION(std::runtime_error("Mir NativeGBMPlatform::create_internal_client is not implemented yet!"));
}

void mgg::NativeGBMPlatform::fill_ipc_package(BufferIPCPacker* packer, Buffer const* buffer) const
{
    auto native_handle = buffer->native_buffer_handle();
    for(auto i=0; i<native_handle->data_items; i++)
    {
        packer->pack_data(native_handle->data[i]);
    }
    for(auto i=0; i<native_handle->fd_items; i++)
    {
        packer->pack_fd(native_handle->fd[i]);
    }

    packer->pack_stride(buffer->stride());
}

extern "C" std::shared_ptr<mg::NativePlatform> create_native_platform(std::shared_ptr<mg::DisplayReport> const& /*report*/)
{
    return std::make_shared<mgg::NativeGBMPlatform>();
}
