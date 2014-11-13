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
#include "display_helpers.h"
#include "drm_close_threadsafe.h"
#include "ipc_operations.h"

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
    drm{drm}
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

std::shared_ptr<mg::PlatformIPCPackage> mgm::IpcOperations::connection_ipc_package()
{
    return std::make_shared<MesaPlatformIPCPackage>(drm->get_authenticated_fd());
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

    drm->auth_magic(magic);

    return mg::PlatformIPCPackage{{0},{}};
}
