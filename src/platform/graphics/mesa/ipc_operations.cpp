/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/buffer.h"
#include "mir/graphics/buffer_ipc_message.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/graphics/nested_context.h"
#include "display_helpers.h"
#include "drm_close_threadsafe.h"
#include "ipc_operations.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;

namespace
{
struct MesaPlatformIPCPackage : public mg::PlatformIPCPackage
{
    MesaPlatformIPCPackage(int drm_auth_fd)
    {
        ipc_fds.push_back(drm_auth_fd);
    }

    ~MesaPlatformIPCPackage()
    {
        if (ipc_fds.size() > 0 && ipc_fds[0] >= 0)
            mgm::drm_close_threadsafe(ipc_fds[0]);
    }
};
}

mgm::IpcOperations::IpcOperations(std::shared_ptr<helpers::DRMHelper> const& drm) :
    drm{drm},
    nested_context{nullptr},
    master{true}
{
}

mgm::IpcOperations::IpcOperations(
    std::shared_ptr<helpers::DRMHelper> const& drm,
        std::shared_ptr<NestedContext> const& nested_context) :
    drm(drm), nested_context(nested_context),master(false)
{
}

void mgm::IpcOperations::pack_buffer(
    mg::BufferIpcMessage& packer, Buffer const& buffer, BufferIpcMsgType msg_type) const
{
    if (msg_type == mg::BufferIpcMsgType::full_msg)
    {
        auto native_handle = buffer.native_buffer_handle();
        for(auto i=0; i<native_handle->data_items; i++)
        {
            packer.pack_data(native_handle->data[i]);
        }
        for(auto i=0; i<native_handle->fd_items; i++)
        {
            packer.pack_fd(mir::Fd(IntOwnedFd{native_handle->fd[i]}));
        }

        packer.pack_stride(buffer.stride());
        packer.pack_flags(native_handle->flags);
        packer.pack_size(buffer.size());
    }
}

void mgm::IpcOperations::unpack_buffer(BufferIpcMessage&, Buffer const&) const
{
}

mg::PlatformIPCPackage mgm::IpcOperations::platform_operation(
    unsigned int const, mg::PlatformIPCPackage const& request)
{
    int magic{0};
    if (request.ipc_data.size() > 0)
        magic = request.ipc_data[0];

    if (master)
        drm->auth_magic(magic);
    else
        nested_context->drm_auth_magic(magic);

    return mg::PlatformIPCPackage{{0},{}};
}

std::shared_ptr<mg::PlatformIPCPackage> mgm::IpcOperations::connection_ipc_package()
{
    if (master)
    {
        return std::make_shared<MesaPlatformIPCPackage>(drm->get_authenticated_fd());
    }
    else
    {
        //TODO: very close to the code in mgmh::DRMHelper::get_authenticated_fd()
        struct MesaNativePlatformIPCPackage : public mg::PlatformIPCPackage
        {
            MesaNativePlatformIPCPackage(int fd)
            {
                ipc_fds.push_back(fd);
            }
        };
        char* busid = drmGetBusid(drm->fd);
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
}
