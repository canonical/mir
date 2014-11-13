/*
 * Copyright © 2013 Canonical Ltd.
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
 * Eleni Maria Stea <elenimaria.stea@canonical.com>
 * Alan Griffiths <alan@octopull.co.uk>
 */

#include "native_platform.h"

#include "buffer_allocator.h"
#include "buffer_writer.h"
#include "mir/graphics/buffer_ipc_message.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/graphics/nested_context.h"

#include "internal_client.h"
#include "internal_native_display.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include <mutex>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mgm = mg::mesa;

mgm::NativePlatform::NativePlatform(std::shared_ptr<NestedContext> const& nested_context_arg)
{
    nested_context = nested_context_arg;
    auto fds = nested_context->platform_fd_items();
    drm_fd = fds.at(0);
    gbm.setup(drm_fd);
    nested_context->drm_set_gbm_device(gbm.device);
}

mgm::NativePlatform::~NativePlatform()
{
    finish_internal_native_display();
}

std::shared_ptr<mg::GraphicBufferAllocator> mgm::NativePlatform::create_buffer_allocator()
{
    return std::make_shared<mgm::BufferAllocator>(gbm.device, mgm::BypassOption::prohibited);
}

std::shared_ptr<mg::PlatformIPCPackage> mgm::NativePlatform::connection_ipc_package()
{
    struct MesaNativePlatformIPCPackage : public mg::PlatformIPCPackage
    {
        MesaNativePlatformIPCPackage(int fd)
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

    return std::make_shared<MesaNativePlatformIPCPackage>(auth_fd);
}

std::shared_ptr<mg::InternalClient> mgm::NativePlatform::create_internal_client()
{
    auto nd = ensure_internal_native_display(connection_ipc_package());
    return std::make_shared<mgm::InternalClient>(nd);
}

/* TODO : this is just a duplication of mgm::BufferPacker::pack_buffer */
void mgm::NativePlatform::fill_buffer_package(
    BufferIpcMessage* message, Buffer const* buffer, BufferIpcMsgType msg_type) const
{
    if (msg_type == mg::BufferIpcMsgType::full_msg)
    {
        auto native_handle = buffer->native_buffer_handle();
        for(auto i=0; i<native_handle->data_items; i++)
        {
            message->pack_data(native_handle->data[i]);
        }
        for(auto i=0; i<native_handle->fd_items; i++)
        {
            message->pack_fd(mir::Fd(IntOwnedFd{native_handle->fd[i]}));
        }

        message->pack_stride(buffer->stride());
        message->pack_flags(native_handle->flags);
        message->pack_size(buffer->size());
    }
}

extern "C" std::shared_ptr<mg::NativePlatform> create_native_platform(
    std::shared_ptr<mg::DisplayReport> const&,
    std::shared_ptr<mg::NestedContext> const& nested_context)
{
    return std::make_shared<mgm::NativePlatform>(nested_context);
}

namespace
{
std::shared_ptr<mgm::InternalNativeDisplay> native_display = nullptr;
std::mutex native_display_guard;
}

bool mgm::NativePlatform::internal_native_display_in_use()
{
    std::unique_lock<std::mutex> lg(native_display_guard);
    return native_display != nullptr;
}

std::shared_ptr<mgm::InternalNativeDisplay> mgm::NativePlatform::internal_native_display()
{
    std::unique_lock<std::mutex> lg(native_display_guard);
    return native_display;
}

std::shared_ptr<mgm::InternalNativeDisplay> mgm::NativePlatform::ensure_internal_native_display(
    std::shared_ptr<mg::PlatformIPCPackage> const& package)
{
    std::unique_lock<std::mutex> lg(native_display_guard);
    if (!native_display)
        native_display = std::make_shared<mgm::InternalNativeDisplay>(package);
    return native_display;
}

void mgm::NativePlatform::finish_internal_native_display()
{
    std::unique_lock<std::mutex> lg(native_display_guard);
    native_display.reset();
}

std::shared_ptr<mg::BufferWriter> mgm::NativePlatform::make_buffer_writer()
{
    return std::make_shared<mgm::BufferWriter>();
}
